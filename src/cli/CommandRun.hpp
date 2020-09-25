/*
 * Sarus
 *
 * Copyright (c) 2018-2020, ETH Zurich. All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef cli_CommandRun_hpp
#define cli_CommandRun_hpp

#include <iostream>
#include <stdexcept>
#include <chrono>

#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include "common/Utility.hpp"
#include "common/Config.hpp"
#include "common/CLIArguments.hpp"
#include "cli/Utility.hpp"
#include "cli/Command.hpp"
#include "cli/MountParser.hpp"
#include "cli/HelpMessage.hpp"
#include "runtime/Runtime.hpp"


namespace sarus {
namespace cli {

class CommandRun : public Command {
public:
    CommandRun() {
        initializeOptionsDescription();
    }

    CommandRun(const std::deque<common::CLIArguments>& argsGroups, std::shared_ptr<common::Config> conf)
        : conf{std::move(conf)}
    {
        initializeOptionsDescription();
        parseCommandArguments(argsGroups);
    }

    void execute() override {
        cli::utility::printLog("Executing run command", common::LogLevel::INFO);

        if(conf->commandRun.enableSSH && !checkUserHasSshKeys()) {
            auto message = boost::format("Failed to check the SSH keys. Hint: try to"
                                         " generate the SSH keys with 'sarus ssh-keygen'.");
            common::Logger::getInstance().log(message, "CLI", common::LogLevel::GENERAL, std::cerr);
            SARUS_THROW_ERROR(message.str(), common::LogLevel::INFO);
        }

        verifyThatImageIsAvailable();

        auto setupBegin = std::chrono::high_resolution_clock::now();
        auto cliTime = std::chrono::duration<double>(setupBegin - conf->program_start);
        auto message = boost::format("Processed CLI arguments in %.6f seconds") % cliTime.count();
        cli::utility::printLog(message, common::LogLevel::INFO);

        auto runtime = runtime::Runtime{conf};
        runtime.setupOCIBundle();

        auto setupEnd = std::chrono::high_resolution_clock::now();
        auto setupTime = std::chrono::duration<double>(setupEnd - setupBegin);
        message = boost::format("successfully set up container in %.6f seconds") % setupTime.count();
        cli::utility::printLog(message, common::LogLevel::INFO);

        auto config_json = boost::filesystem::path{conf->json["OCIBundleDir"].GetString()} / "config.json";

        auto gid = conf->userIdentity.gid;
        auto uid = conf->userIdentity.uid;

        if(chown(config_json.c_str(), uid, gid) != 0) {
            auto errorMessage = boost::format("failed to change ownership of path: %s") % config_json;
            SARUS_THROW_ERROR(errorMessage.str());
        }

        if(setresgid(gid, gid, gid) != 0) {
            auto message = boost::format("Failed to setresgid(%1%, %1%, %1%): %2%") % gid % strerror(errno);
            SARUS_THROW_ERROR(message.str());
        }

        if(setresuid(uid, uid, uid) != 0) {
            auto message = boost::format("Failed to setresuid(%1%, %1%, %1%): %2%") % uid % strerror(errno);
            SARUS_THROW_ERROR(message.str());
        }

        runtime.executeContainer();

        cli::utility::printLog("Successfully executed run command", common::LogLevel::INFO);
    }

    bool requiresRootPrivileges() const override {
        return true;
    }

    std::string getBriefDescription() const override {
        return  "Run a command in a new container";
    }

    void printHelpMessage() const override {
        auto printer = cli::HelpMessage()
            .setUsage("sarus run [OPTIONS] REPOSITORY[:TAG] [COMMAND] [ARG...]\n"
                "\n"
                "Note: REPOSITORY[:TAG] has to be specified as\n"
                "      displayed by the \"sarus images\" command.")
            .setDescription(getBriefDescription())
            .setOptionsDescription(optionsDescription);
        std::cout << printer;
    }

private:
    void initializeOptionsDescription() {
        optionsDescription.add_options()
            ("centralized-repository", "Use centralized repository instead of the local one")
            ("entrypoint",
                boost::program_options::value<std::string>(&entrypoint)->implicit_value(""),
                "Overwrite the default ENTRYPOINT of the image")
            ("glibc", "Enable replacement of the container's GNU C libraries")
            ("init", "Run an init process inside the container that forwards signals and reaps processes")
            ("mount",
                boost::program_options::value<std::vector<std::string>>(&conf->commandRun.userMounts),
                "Mount custom directories into the container")
            ("mpi,m", "Enable MPI support. Implies '--glibc'")
            ("ssh", "Enable SSH in the container")
            ("tty,t", "Allocate a pseudo-TTY in the container")
            ("workdir,w",
                boost::program_options::value<std::string>(&workdir),
                "Set working directory inside the container");
    }

    void parseCommandArguments(const std::deque<common::CLIArguments>& argsGroups) {
        cli::utility::printLog("parsing CLI arguments of run command", common::LogLevel::DEBUG);

        // the run command expects at least two arguments (run + image)
        if(argsGroups.size() < 2) {
            auto message = boost::format("Bad number of arguments for command 'run'"
                                        "\nSee 'sarus help run'");
            utility::printLog(message, common::LogLevel::GENERAL, std::cerr);
            SARUS_THROW_ERROR(message.str(), common::LogLevel::INFO);
        }

        try {
            // Parse options
            boost::program_options::variables_map values;
            boost::program_options::parsed_options parsed =
                boost::program_options::command_line_parser(argsGroups[0].argc(), argsGroups[0].argv())
                        .options(optionsDescription)
                        .run();
            boost::program_options::store(parsed, values);
            boost::program_options::notify(values);

            conf->imageID = cli::utility::parseImageID(argsGroups[1]);
            conf->useCentralizedRepository = values.count("centralized-repository");
            conf->directories.initialize(conf->useCentralizedRepository, *conf);
            // the remaining arguments (after image) are all part of the command to be executed in the container
            conf->commandRun.execArgs = std::accumulate(argsGroups.cbegin()+2, argsGroups.cend(), common::CLIArguments{});


            if(values.count("entrypoint")) {
                if(entrypoint.empty()) {
                    conf->commandRun.entrypoint = common::CLIArguments{};
                }
                else {
                    conf->commandRun.entrypoint = common::CLIArguments{entrypoint};
                }
            }
            if(values.count("glibc")) {
                conf->commandRun.enableGlibcReplacement = true;
            }
            if(values.count("init")) {
                conf->commandRun.addInitProcess = true;
            }
            if(values.count("mpi")) {
                conf->commandRun.useMPI = true;
            }
            if(values.count("ssh")) {
                conf->commandRun.enableSSH = true;
            }
            if(values.count("tty")) {
                conf->commandRun.allocatePseudoTTY = true;
            }
            if(values.count("workdir")) {
                conf->commandRun.workdir = workdir;
                if(!conf->commandRun.workdir->is_absolute()) {
                    auto message = boost::format("The working directory '%s' is invalid, it"
                                                 " needs to be an absolute path.") % workdir;
                    SARUS_THROW_ERROR(message.str());
                }
            }
        }
        catch (std::exception& e) {
            auto message = boost::format("%s\nSee 'sarus help run'") % e.what();
            cli::utility::printLog(message, common::LogLevel::GENERAL, std::cerr);
            SARUS_THROW_ERROR(message.str(), common::LogLevel::INFO);
        }

        makeSiteMountObjects();
        makeUserMountObjects();

        cli::utility::printLog("successfully parsed CLI arguments", common::LogLevel::DEBUG);
    }

    void makeSiteMountObjects() {
        bool isUserMount = false;
        auto parser = cli::MountParser{isUserMount, conf};
        for(const auto& map : convertJSONSiteMountsToMaps()) {
            conf->commandRun.mounts.push_back(parser.parseMountRequest(map));
        }
    }

    void makeUserMountObjects() {
        bool isUserMount = true;
        auto parser = cli::MountParser{isUserMount, conf};
        for (const auto& mountString : conf->commandRun.userMounts) {
            auto map = common::parseMap(mountString);
            conf->commandRun.mounts.push_back(parser.parseMountRequest(map));
        }
    }

    std::vector<std::unordered_map<std::string, std::string>> convertJSONSiteMountsToMaps() {
        auto maps = std::vector<std::unordered_map<std::string, std::string>>{};

        try {
            if(!conf->json.HasMember("siteMounts")) {
                return maps;
            }

            for(const auto& mount : conf->json["siteMounts"].GetArray()) {
                auto map = std::unordered_map<std::string, std::string>{};
                for(const auto& field : mount.GetObject()) {
                    if(field.name.GetString() == std::string{"flags"}) {
                        for(const auto& flag : field.value.GetObject()) {
                            map[flag.name.GetString()] = flag.value.GetString();
                        }
                    }
                    else {
                        map[field.name.GetString()] = field.value.GetString();
                    }
                }
                maps.push_back(std::move(map));
            }
        } catch(const std::exception& e) {
            SARUS_RETHROW_ERROR(e, "Failed to convert JSON mount entry to map");
        }

        return maps;
    }

    bool checkUserHasSshKeys() const {
        cli::utility::printLog( "Checking that the user has SSH keys",
                                common::LogLevel::INFO);

        common::setEnvironmentVariable("HOOK_BASE_DIR=" + std::string{conf->json["localRepositoryBaseDir"].GetString()});

        auto passwdFile = boost::filesystem::path{ conf->json["prefixDir"].GetString() } / "etc/passwd";
        common::setEnvironmentVariable("PASSWD_FILE=" + passwdFile.string());

        auto args = common::CLIArguments{
            std::string{ conf->json["prefixDir"].GetString() } + "/bin/ssh_hook",
            "check-user-has-sshkeys"
        };

        auto setUserIdentity = [this]() {
            auto gid = conf->userIdentity.gid;
            if(setresgid(gid, gid, gid) != 0) {
                auto message = boost::format("Failed to setresgid(%1%, %1%, %1%): %2%") % gid % strerror(errno);
                SARUS_THROW_ERROR(message.str());
            }

            auto uid = conf->userIdentity.uid;
            if(setresuid(uid, uid, uid) != 0) {
                auto message = boost::format("Failed to setresuid(%1%, %1%, %1%): %2%") % uid % strerror(errno);
                SARUS_THROW_ERROR(message.str());
            }
        };

        return common::forkExecWait(args, std::function<void()>{setUserIdentity}) == 0;
    }

    void verifyThatImageIsAvailable() const {
        cli::utility::printLog( boost::format("Verifying that image %s is available") % conf->getImageFile(),
                                common::LogLevel::INFO);
        if(!boost::filesystem::exists(conf->getImageFile())) {
            auto message = boost::format("Specified image %s is not available") % conf->imageID;
            cli::utility::printLog(message.str(), common::LogLevel::GENERAL, std::cerr);
            exit(EXIT_FAILURE);
        }
        cli::utility::printLog("Successfully verified that image is available", common::LogLevel::INFO);
    }

private:
    boost::program_options::options_description optionsDescription{"Options"};
    std::shared_ptr<common::Config> conf;
    std::string entrypoint;
    std::string workdir;
};

}
}

#endif
