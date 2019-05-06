# Sarus - An OCI-compatible container engine for HPC

<a target="_blank" href="https://opensource.org/licenses/BSD-3-Clause">![License: BSD][BSD.License]</a>

Sarus is a software to run Linux containers on High Performance Computing
environments. Its development has been driven by the specific requirements of
HPC systems, while leveraging open standards and technologies to encourage
vendor and community involvement.

Key features:

* Spawning of isolated software environments (*containers*), built by users to
  fit the deployment of a specific application
* Security oriented to HPC systems
* Extensible runtime by means of OCI hooks to allow current and future
  support of custom hardware while achieving native performance
* Creation of container filesystems tailored for diskless nodes and parallel
  filesystems
* Compatibility with the presence of a workload manager
* Compatibility with the Open Container Initiative (OCI) standards:

    - Can pull images from registries adopting the OCI Distribution
      Specification or the Docker Registry HTTP API V2 protocol
    - Can import and convert images adopting the OCI Image Format
    - Sets up a container bundle complying to the OCI Runtime Specification
    - Uses an OCI-compliant runtime to spawn the container process


## Building the documentation

The full documentation is located in the `doc` directory and can be built using
Python 3 and Sphinx:

```
cd doc
python3 -m venv ./venv
source venv/bin/activate
pip install sphinx sphinx-rtd-theme
make html
```
