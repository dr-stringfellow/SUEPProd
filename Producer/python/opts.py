from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.register('config', default = '', mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.string, info = 'Single-switch config. Values: Prompt17, Summer16')
options.register('globaltag', default = '', mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.string, info = 'Global tag')
options.register('pdfname', default = '', mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.string, info = 'PDF name')
options.register('redojec', default = False, mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.bool, info = 'Redo JEC')
options.register('connect', default = '', mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.string, info = 'Globaltag connect')
options.register('lumilist', default = '', mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.string, info = 'Good lumi list JSON')
options.register('isData', default = False, mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.bool, info = 'True if running on Data, False if running on MC')
options.register('useTrigger', default = True, mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.bool, info = 'Fill trigger information')
options.register('printLevel', default = 0, mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.int, info = 'Debug level of the ntuplizer')
options.register('skipEvents', default = 0, mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.int, info = 'Skip first events')
options.register('dumpPython', default = False, mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.bool, info = 'Dumps configuration as single python file to stdout')
options._tags.pop('numEvent%d')
options._tagOrder.remove('numEvent%d')

options.parseArguments()
