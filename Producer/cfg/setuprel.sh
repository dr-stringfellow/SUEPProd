#!/bin/bash

# Use SUEPProd/Producer/scripts/cms-git-diff to find out which packages should be checked out from each branch.

CWD=`pwd`
SRC=$CMSSW_BASE/src
INSTALL=$CMSSW_BASE/src/SUEPProd/Producer/scripts/install-pkg


# Cut-based electron and photon ID
$INSTALL lsoffi CMSSW_9_4_0_pre3_TnP RecoEgamma/ElectronIdentification RecoEgamma/PhotonIdentification

# MVA electron ID
# RecoEgamma/ElectronIdentification overlaps with the line above but most of the files added above are not contained in this branch and thus will not be overwritten
$INSTALL guitargeek ElectronID_MVA2017_940pre3 RecoEgamma/EgammaTools RecoEgamma/ElectronIdentification -x RecoEgamma/ElectronIdentification/python/Identification/cutBasedElectronID_tools.py
# This is getting ridiculous (difference between 94X and 101X is purely technical)
cp $CMSSW_RELEASE_BASE/src/RecoEgamma/EgammaTools/src/EcalClusterLocal.cc $SRC/RecoEgamma/EgammaTools/src/EcalClusterLocal.cc

# MVA weights for ID
git clone -b CMSSW_9_4_0_pre3_TnP https://github.com/lsoffi/RecoEgamma-ElectronIdentification.git electronid
cp -r electronid/* $SRC/RecoEgamma/ElectronIdentification/data/
rm -rf electronid
git clone -b CMSSW_9_4_0_pre3_TnP https://github.com/lsoffi/RecoEgamma-PhotonIdentification.git photonid
cp -r photonid/* $SRC/RecoEgamma/PhotonIdentification/data/
rm -rf photonid

# Electron and photon calibration
$INSTALL cms-egamma EGM_94X_v1 EgammaAnalysis/ElectronTools

# MVA weights
cd $SRC/EgammaAnalysis/ElectronTools/data
git clone https://github.com/ECALELFS/ScalesSmearings.git
cd ScalesSmearings/
git checkout Run2017_17Nov2017_v1
cd $CWD

# Jet & MET reclustering (maybe not strictly needed in our current prod.py because we don't recluster)
$INSTALL cms-met METRecipe94x PhysicsTools/PatAlgos PhysicsTools/PatUtils

# Deep double B tagger from FNAL
#$INSTALL jmduarte double-b-rebased-94x  RecoBTag/Combined RecoBTag/Configuration RecoBTag/DeepFlavour DataFormats/BTauReco PhysicsTools/PatAlgos
$INSTALL DylanHsu double-b-rebased-94x  RecoBTag/Combined RecoBTag/Configuration RecoBTag/DeepFlavour DataFormats/BTauReco PhysicsTools/PatAlgos

# Last contribtion to PhysicsTools/PatAlgos from cms-met/METRecipe94x was on Nov 6, 2017.
# It was merged into CMSSW as commit 8a9523901e3fc5cec4c6200ed2160e0f45baca7d, so this should be safe
cp $CMSSW_RELEASE_BASE/src/PhysicsTools/PatAlgos/plugins/* $SRC/PhysicsTools/PatAlgos/plugins/.

cp $CMSSW_RELEASE_BASE/src/PhysicsTools/PatAlgos/interface/MuonMvaEstimator.h $SRC/PhysicsTools/PatAlgos/interface/.
cp $CMSSW_RELEASE_BASE/src/PhysicsTools/PatAlgos/src/MuonMvaEstimator.cc $SRC/PhysicsTools/PatAlgos/src/.
cp $CMSSW_RELEASE_BASE/src/PhysicsTools/PatAlgos/interface/SoftMuonMvaEstimator.h $SRC/PhysicsTools/PatAlgos/interface/.
cp $CMSSW_RELEASE_BASE/src/PhysicsTools/PatAlgos/src/SoftMuonMvaEstimator.cc $SRC/PhysicsTools/PatAlgos/src/.


# clone to a temp area and move the contents to avoid including .git
TEMP=$(mktemp -d)
git clone https://github.com/jmduarte/RecoBTag-Combined.git $TEMP/data -b deepdoubleb_v0
mkdir -p $SRC/RecoBTag/Combined/data
mv $TEMP/data/DeepDoubleB $SRC/RecoBTag/Combined/data/
rm -rf $TEMP
