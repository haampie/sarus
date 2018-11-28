#include "common/Utility.hpp"
#include "common/PathRAII.hpp"
#include "image_manager/SquashfsImage.hpp"
#include "test_utility/config.hpp"
#include "test_utility/unittest_main_function.hpp"

namespace sarus {
namespace image_manager {
namespace test {

TEST_GROUP(SquashfsImageTestGroup) {
};

TEST(SquashfsImageTestGroup, testSquashfsImage) {
    auto config = test_utility::config::makeConfig();
    config.imageID = {"server", "repositoryNamespace", "image", "tag"};

    common::PathRAII repository{config.directories.repository};
    boost::filesystem::remove_all(repository.getPath());

    common::PathRAII expandedImage{common::makeUniquePathWithRandomSuffix("/tmp/sarus-test-expandedImage")};
    common::createFoldersIfNecessary(expandedImage.getPath());

    SquashfsImage{config, expandedImage.getPath(), config.getImageFile()};

    CHECK(boost::filesystem::exists(config.getImageFile()));
}

}}} // namespace

SARUS_UNITTEST_MAIN_FUNCTION();
