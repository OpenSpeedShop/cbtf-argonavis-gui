# cbtf-argonavis-gui

Baseline for next generation Open|SpeedShop Graphical User Interface (GUI).

The primary focus of this GUI will be the processing and display of CUDA collector performance data.  However, there will be refactoring phases to adopt the GUI to support the processing and display of any collector performance data.

Other collectors with good support so far are:

- usertime
- mpi
- pthreads


## Building the developmental Open|SpeedShop GUI

The developmental Open|SpeedShop GUI depends on various libraries provided by other Open|SpeedShop GitHub repositories.  Also, for calltree view support (which is required), Graphviz development header files and libraries must be available on the target machine.

Thus, the following environment variables need to be defined before building the software:

```
export CBTF_ROOT=<the location specified by "--cbtf-install-prefix" for build of git@github.com:OpenSpeedShop/cbtf.git repostory>
export OSS_CBTF_ROOT=<the location specified by "--openss-prefix" for build of git@github.com:OpenSpeedShop/openspeedshop.git repostory>
export KRELL_ROOT=<the location specified by "--krell-root-prefix" for build of git@github.com:OpenSpeedShop/openspeedshop.git repostory>
export BOOST_ROOT=<the location of Boost development header files and libraries>
export GRAPHVIZ_ROOT=<the location of GraphViz development header files and libraries>
```

If the QtGraph library was not previously built and installed for the particular Qt version for building the developmental Open|SpeedShop GUI, then refer to the QtGraph repository located here:

https://github.com/OpenSpeedShop/QtGraph

The QtGraph source-code can be obtained as follows:

```
git clone git@github.com:OpenSpeedShop/QtGraph.git
```

The following information regarding GraphViz should already have been done in order to build the QtGraph library.

NOTE:  If GraphViz needs to be built from source, the latest stable snapshot tarball can be found here:

http://www.graphviz.org/pub/graphviz/stable/SOURCES/graphviz-2.40.1.tar.gz

For NASA HECC machines, the Graphviz development environment can be found in one of the 'pkgsrc' packages.  For example:

```
module load pkgsrc/2016Q2
```

Thus, the GRAPHVIZ_ROOT environment variable would be the following:

```
export GRAPHVIZ_ROOT=/nasa/pkgsrc/2016Q2
```

For Ubuntu 14.04 or 16.04 LTS systems, the Graphviz develop environment can be installed via:

```
sudo apt-get install libgraphviz-dev
```

Once the software dependencies have been provided, the following commands can be executed:

```
qmake && make
```

Screenshots
-----------

1. **CUDA Event Timeline (comparison with NVIDIA Visual Performance Profiler)**

![Alt text](/../screenshots/images/Screenshot4.png?raw=true "Open|SpeedShop CUDA Event Timeline")

![Alt text](/../screenshots/images/Screenshot3.png?raw=true "NVIDIA Visual Performance Profiler Timeline")


2. **CUDA Event Details View**

![Alt text](/../screenshots/images/Screenshot5.png?raw=true "All CUDA Event Details")

![Alt text](/../screenshots/images/Screenshot6.png?raw=true "CUDA Kernel Execution Details")

![Alt text](/../screenshots/images/Screenshot7.png?raw=true "CUDA Data Transfer Details")


3. **Metric Views Cross-Referenced To Source-Code**

![Alt text](/../screenshots/images/Screenshot8.png?raw=true "Metrics w/Source-Code View")


4. **Calltree View**

![Alt text](/../screenshots/images/Screenshot1.png?raw=true "Complex Calltree")

![Alt text](/../screenshots/images/Screenshot2.png?raw=true "Simple Calltree")


5. **MPI Trace Timeline**

![Alt text](/../screenshots/images/Screenshot18.png?raw=true "Simple MPI Program") | ![Alt text](/../screenshots/images/Screenshot19.png?raw=true "Simple MPI Program")
------------ | -------------
![Alt text](/../screenshots/images/Screenshot20.png?raw=true "MPI nbody - MPICH on Cray") | ![Alt text](/../screenshots/images/Screenshot21.png?raw=true "MPI nbody - Linux Laptop - Default View")
![Alt text](/../screenshots/images/Screenshot22.png?raw=true "Experiment Subrange - MPI nbody") | ![Alt text](/../screenshots/images/Screenshot23.png?raw=true "Experiment Subrange - MPI nbody")


Miscellaneous Additional Screenshots
------------------------------------

1. **Open|SpeedShop Experiment of LAMMPS Simulation with KOKKOS package for GPUs**


![Alt text](/../screenshots/images/Screenshot10.png?raw=true "Experiment Overview") | ![Alt text](/../screenshots/images/Screenshot11.png?raw=true "Beginning of Experiment View")
------------ | -------------
![Alt text](/../screenshots/images/Screenshot12.png?raw=true "Experiment Subrange - CUDA Events") | ![Alt text](/../screenshots/images/Screenshot13.png?raw=true "Experiment Subrange - CUDA Kernel Execution Events")
![Alt text](/../screenshots/images/Screenshot14.png?raw=true "Experiment Subrange - CUDA Events") | ![Alt text](/../screenshots/images/Screenshot15.png?raw=true "Experiment Subrange - CUDA Events")
![Alt text](/../screenshots/images/Screenshot16.png?raw=true "Experiment Subrange - CUDA Events") | ![Alt text](/../screenshots/images/Screenshot17.png?raw=true "Experiment Subrange - CUDA Data Transfer Events")
