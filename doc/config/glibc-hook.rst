******************
Native glibc hook
******************

Sarus's source code includes a hook able to inject glibc libraries from the
host inside the container, overriding the glibc of the container.

This is useful in case that we need to upgrade the container's glibc to a newer
version. For example, when we want to inject a host's library inside the container
(e.g. MPI), but the host's library relies on a newer glibc than the container's one.

When activated, the hook will enter the mount namespace of the container, search
for dynamically-linkable glibc libraries and replace them with functional
equivalents bind-mounted from the host system.

In order for the replacements to work seamlessly, the hook does the following:

* Compare glibc versions of host and container. The container's libraries are only
  replaced when they are older than the host's libraries.

* Check ABI compatibility between host and container glibc.
  Host and container glibc libraries must have the same soname.

Hook installation
=================

The hook is written in C++ and it will be compiled when building Sarus without
the need of additional dependencies. Sarus's installation scripts will also
automatically install the hook in the ``$CMAKE_INSTALL_PREFIX/bin`` directory.
In short, no specific action is required to install the glibc hook.

Sarus configuration
=====================

The program is meant to be run as a **prestart** hook and does not accept
arguments, but its actions are controlled through a few environment variables:

* ``GLIBC_LIBS``: Colon separated list of full paths to the host's glibc
  libraries that will substitute the container's libraries.

* ``LDCONFIG_PATH``: Absolute path to a trusted ``ldconfig``
  program **on the host**.

* ``READELF_PATH``: Absolute path to a trusted ``readelf``
  program **on the host**.

The following is an example ``OCIHooks`` object enabling the glibc hook:

.. code-block:: json


    {
        "prestart": [
            {
                "path": "/opt/sarus/bin/glibc_hook",
                "env": [
                    "LDCONFIG_PATH=/sbin/ldconfig",
                    "READELF_PATH=/usr/bin/readelf",
                    "GLIBC_LIBS=/lib64/libSegFault.so:/lib64/librt.so.1:/lib64/libnss_dns.so.2:/lib64/libanl.so.1:/lib64/libresolv.so.2:/lib64/libnsl.so.1:/lib64/libBrokenLocale.so.1:/lib64/ld-linux-x86-64.so.2:/lib64/libnss_hesiod.so.2:/lib64/libutil.so.1:/lib64/libnss_files.so.2:/lib64/libnss_compat.so.2:/lib64/libnss_db.so.2:/lib64/libm.so.6:/lib64/libcrypt.so.1:/lib64/libc.so.6:/lib64/libpthread.so.0:/lib64/libdl.so.2:/lib64/libmvec.so.1:/lib64/libthread_db.so.1"
                ]
            }
        ]
    }

Sarus support at runtime
========================

The glibc hook injects the host glibc in the container filesystem only if the annotation
``com.hooks.glibc.enabled=true`` is present in the config.json of the OCI bundle.
The annotation is automatically generated by Sarus if the ``--glibc`` or
``--mpi`` command line options are passed to :program:`sarus run`.

