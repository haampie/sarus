SSH hook
========

Sarus also includes the source code for a hook capable of enabling SSH
connections inside containers. The SSH hook is an executable binary that
performs different ssh-related operations depending on the argument it
receives from the runtime. For the full details about the implementation and
inner workings, please refer to the related :doc:`developer documentation
</developer/ssh>`.

Hook installation
-----------------

The hook is written in C++ and it will be compiled along with Sarus if the
``ENABLE_SSH=TRUE`` CMake option has been used when configuring the build (the
option is enabled by default). The Sarus installation scripts will also
automatically install the hook in the ``<CMAKE_INSTALL_PREFIX>/bin`` directory.

A custom OpenSSH software will also be built and installed in the
``<CMAKE_INSTALL_PREFIX>/openssh`` directory. This directory must satisfy the
:ref:`security requirements <post-installation-permissions-security>` for critical
files and directories.

Sarus configuration
---------------------

The SSH hook must be configured to run as a **prestart** hook. It expects to
receive its own name/location as the first argument, and the string
``start-sshd`` as positional argument. In addition, the following
environment variables must be defined:

* ``HOOK_BASE_DIR``: Absolute base path to the directory where the hook will create and access the SSH keys.
  The keys directory will be located in ``<HOOK_BASE_DIR>/<username>/.oci-hooks/ssh/keys``.

* ``PASSWD_FILE``: Absolute path to a password file (PASSWD(5)).
  The file is used by the hook to retrieve the username of the user.

* ``OPENSSH_DIR``: Absolute path to the location of the custom OpenSSH software.

The following is an example ``OCIHooks`` object enabling the SSH hook:

.. code-block:: json

    {
        "prestart": [
            {
                "path": "/opt/sarus/bin/ssh_hook",
                "env": [
                    "HOOK_BASE_DIR=/home",
                    "PASSWD_FILE=/opt/sarus/etc/passwd",
                    "OPENSSH_DIR=/opt/sarus/openssh"
                ],
                "args": [
                    "ssh_hook",
                    "start-sshd"
                ]
            }
        ]
    }

Sarus support at runtime
------------------------

The command ``sarus ssh-keygen`` will call the hook without creating a
container, passing the appropriate arguments to generate dedicated keys to be
used by containers.

The hook bind-mounts the custom OpenSSH software and starts the sshd daemon only if the
annotation ``com.hooks.ssh.enabled=true`` is present in the config.json of the OCI bundle.
The annotation is automatically generated by Sarus if the ``--ssh``
command line option is passed to :program:`sarus run`.
