from CMGTools.RootTools.analyzers.TreeAnalyzerNumpy import TreeAnalyzerNumpy
from CMGTools.GenStudies.analyzers.ntuple import *

def var( tree, varName, type=float ):
    tree.var(varName, type)

def fill( tree, varName, value ):
    tree.fill( varName, value )


class MultiTreeProducer( TreeAnalyzerNumpy ):
    
    def declareVariables(self):

        tr = self.tree
        bookGenParticle(tr, 'pdgId')

    def process(self, iEvent, event):
        
        tr = self.tree
        tr.reset()



        for io in range(len(event.genParticles)):
            fillGenParticle(tr, 'pdgId', event.genParticles[io])
#            fill(tr, 'pdgId', event.genParticles[io])
            self.tree.tree.Fill()
       
