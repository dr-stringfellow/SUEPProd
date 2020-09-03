# SUEPProd
Package for production of PandA from CMSSW

Installation

```bash
    cd $CMSSW_BASE/src
    eval `scram runtime -sh`
    
    git clone https://github.com/SUEPPhysics/SUEPTree
    git clone https://github.com/SUEPPhysics/SUEPProd

    # Don't do the following if you want to use 92x, but it is necessary for 80x:
    ./SUEPProd/Producer/cfg/setuprel.sh

    scram b -j12
```

Run

    cmsRun SUEPProd/Producer/cfg/prod.py [options]
