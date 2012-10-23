import math
from CMGTools.RootTools.fwlite.AutoHandle import AutoHandle
from CMGTools.H2TauTau.proto.analyzers.ntuple import *
from CMGTools.RootTools.physicsobjects.PhysicsObjects import Lepton, Muon, Electron
from CMGTools.RootTools.analyzers.TreeAnalyzerNumpy import TreeAnalyzerNumpy

def var( tree, varName, type=float ):
    tree.var(varName, type)

def fill( tree, varName, value ):
    tree.fill( varName, value )

def bookParticle( tree, pName ):
    var(tree, '{pName}_pt'.format(pName=pName))
    var(tree, '{pName}_eta'.format(pName=pName))
    var(tree, '{pName}_phi'.format(pName=pName))
    var(tree, '{pName}_mass'.format(pName=pName))
    var(tree, '{pName}_charge'.format(pName=pName))

def bookW( tree, pName ):
    var(tree, '{pName}_pt'.format(pName=pName))
    var(tree, '{pName}_phi'.format(pName=pName))

def bookZ( tree, pName ):
    var(tree, '{pName}_pt'.format(pName=pName))
    var(tree, '{pName}_rap'.format(pName=pName))
    var(tree, '{pName}_phi'.format(pName=pName))
    var(tree, '{pName}_mass'.format(pName=pName))

def bookMET( tree, pName ):
    var(tree, '{pName}'.format(pName=pName))
    var(tree, '{pName}_phi'.format(pName=pName))

def bookJet( tree, pName ):
    var(tree, '{pName}_pt'.format(pName=pName))
    var(tree, '{pName}_eta'.format(pName=pName))
    var(tree, '{pName}_phi'.format(pName=pName))

def fillParticle( tree, pName, particle ):
    fill(tree, '{pName}_pt'.format(pName=pName), particle.pt() )
    fill(tree, '{pName}_eta'.format(pName=pName), particle.eta() )
    fill(tree, '{pName}_phi'.format(pName=pName), particle.phi() )
    fill(tree, '{pName}_mass'.format(pName=pName), particle.mass() )
    fill(tree, '{pName}_charge'.format(pName=pName), particle.charge() )

def fillW( tree, pName, particle ):
    fill(tree, '{pName}_pt'.format(pName=pName), particle.Pt() )
    fill(tree, '{pName}_phi'.format(pName=pName), particle.Phi() )

def fillZ( tree, pName, particle ):
    fill(tree, '{pName}_pt'.format(pName=pName), particle.Pt() )
    fill(tree, '{pName}_rap'.format(pName=pName), particle.Rapidity() )
    fill(tree, '{pName}_phi'.format(pName=pName), particle.Phi() )
    fill(tree, '{pName}_mass'.format(pName=pName), particle.M() )

def fillMET( tree, pName, particle ):
    fill(tree, '{pName}'.format(pName=pName), particle.pt() )
    fill(tree, '{pName}_phi'.format(pName=pName), particle.phi() )

def fillJet( tree, pName, particle ):
    fill(tree, '{pName}_pt'.format(pName=pName), particle.pt() )
    fill(tree, '{pName}_eta'.format(pName=pName), particle.eta() )
    fill(tree, '{pName}_phi'.format(pName=pName), particle.phi() )



class ZTreeProducer( TreeAnalyzerNumpy ):
    
    MuonClass = Muon 

    
    def declareHandles(self):
      super(ZTreeProducer, self).declareHandles()
    
      self.handles['pfmet'] = AutoHandle(
        'cmgPFMET',
        'std::vector<cmg::BaseMET>' 
        )
    
      self.handles['muons'] = AutoHandle(
            'cmgMuonSel',
            'std::vector<cmg::Muon>'
            )

      self.handles['electrons'] = AutoHandle(
            'cmgElectronSel',
            'std::vector<cmg::Electron>'
            )
    
      self.mchandles['genParticles'] = AutoHandle( 'genParticlesPruned',
            'std::vector<reco::GenParticle>' )

      self.handles['vertices'] =  AutoHandle(
          'offlinePrimaryVertices',
          'std::vector<reco::Vertex>'
          )
            
    
    def declareVariables(self):
      tr = self.tree

      var( tr, 'run', int)
      var( tr, 'lumi', int)
      var( tr, 'evt', int)
      var( tr, 'nvtx', int)
      var( tr, 'evtHasGoodVtx', int)
      var( tr, 'evtHasTrg', int)
      var( tr, 'evtZSel', int)
      
      var( tr, 'nMuons', int)
      var( tr, 'nTrgMuons', int)
      var( tr, 'nNoTrgMuons', int)

      bookMET(tr, 'pfmet')
      var(tr, 'pfmet_sumEt')
      bookMET(tr, 'pfmetWlikeNeg')
      bookMET(tr, 'pfmetWlikePos')

      bookW(tr, 'WlikePos')
      var(tr, 'WlikePos_mt')
      bookW(tr, 'WlikeNeg')
      var(tr, 'WlikeNeg_mt')
      bookZ(tr, 'Z')
      var(tr, 'Z_mt')
      bookZ(tr, 'ZGen')      
      var(tr, 'ZGen_mt')      
      var( tr, 'u1')
      var( tr, 'u2')
      
      bookParticle(tr, 'MuPos')
      var(tr, 'MuPos_dxy')
      var(tr, 'MuPosRelIso')
      var(tr, 'MuPosTrg', int)
      var(tr, 'MuPosIsTightAndIso', int)
      bookParticle(tr, 'MuPosGen')
      # var(tr, 'MuPosGen_pdgId', int)
      var(tr, 'MuPosDRGenP')
      bookParticle(tr, 'MuNeg')
      var(tr, 'MuNeg_dxy')
      var(tr, 'MuNegRelIso')
      var(tr, 'MuNegTrg', int)
      var(tr, 'MuNegIsTightAndIso', int)
      bookParticle(tr, 'MuNegGen')
      # var(tr, 'MuNegGen_pdgId', int)
      var(tr, 'MuNegDRGenP')
      
      bookJet(tr, 'Jet_leading')
       
    def process(self, iEvent, event):

        self.readCollections( iEvent )
        tr = self.tree
        tr.reset()
        
        # print 'event.savegenpZ= ',event.savegenpZ,' self.cfg_comp.isMC= ',self.cfg_comp.isMC,' event.ZGoodEvent= ',event.ZGoodEvent
        if (event.savegenpZ and self.cfg_comp.isMC):
                  
          fillZ(tr, 'ZGen', event.genZ[0].p4())
          fill(tr, 'ZGen_mt', event.genZ_mt)
          fillParticle(tr, 'MuPosGen', event.genMuPos[0])
          fill(tr, 'MuPosDRGenP', event.muPosGenDeltaRgenP)              
          fillParticle(tr, 'MuNegGen', event.genMuNeg[0])
          fill(tr, 'MuNegDRGenP', event.muNegGenDeltaRgenP)
            
        if event.ZGoodEvent == True :
                                    
          fillZ(tr, 'Z', event.Z4V)
          fill(tr,  'Z_mt', event.Z4V_mt)
          fill(tr, 'u1', event.Zu1)
          fill(tr, 'u2', event.Zu2)
          fillW(tr, 'WlikePos', event.Wpos4VfromZ)
          fill(tr,  'WlikePos_mt', event.Wpos4VfromZ_mt)
          fillMET(tr, 'pfmetWlikePos', event.ZpfmetWpos)
          fillW(tr, 'WlikeNeg', event.Wneg4VfromZ)
          fill(tr,  'WlikeNeg_mt', event.Wneg4VfromZ_mt)
          fillMET(tr, 'pfmetWlikeNeg', event.ZpfmetWneg)
          
          fillParticle(tr, 'MuPos', event.BestZPosMuon)
          if ( event.BestZPosMuon.isGlobalMuon() or event.BestZPosMuon.isTrackerMuon() ) and event.HasGoodVertices:
            fill(tr, 'MuPos_dxy', math.fabs(event.BestZPosMuon.dxy()))
          fill(tr, 'MuPosRelIso', event.BestZPosMuon.relIso(0.5))
          fill(tr, 'MuPosTrg', event.BestZPosMuonHasTriggered)
          fill(tr, 'MuPosIsTightAndIso',event.BestZPosMuonIsTightAndIso)
          fillParticle(tr, 'MuNeg', event.BestZNegMuon)
          if ( event.BestZNegMuon.isGlobalMuon() or event.BestZNegMuon.isTrackerMuon() ) and event.HasGoodVertices:
            fill(tr, 'MuNeg_dxy', math.fabs(event.BestZNegMuon.dxy()))
          fill(tr, 'MuNegRelIso', event.BestZNegMuon.relIso(0.5))
          fill(tr, 'MuNegTrg', event.BestZNegMuonHasTriggered)
          fill(tr, 'MuNegIsTightAndIso',event.BestZNegMuonIsTightAndIso)
          
        if (event.savegenpZ and self.cfg_comp.isMC) or event.ZGoodEvent:
          fill( tr, 'run', event.run) 
          fill( tr, 'lumi',event.lumi)
          fill( tr, 'evt', event.eventId)
          fill( tr, 'nvtx', len(self.handles['vertices'].product()))
          fill( tr, 'nMuons', len(event.ZallMuons))
          fill( tr, 'nTrgMuons', len(event.ZselTriggeredMuons))
          fill( tr, 'nNoTrgMuons', len(event.ZselNoTriggeredMuons))
          fill( tr, 'evtHasGoodVtx', event.HasGoodVertices)
          fill( tr, 'evtHasTrg', event.HasTriggerFired)
          fill( tr, 'evtZSel', event.ZGoodEvent)
          fillMET(tr, 'pfmet', event.ZpfmetNoMu)
          fill(tr, 'pfmet_sumEt', event.pfmet.sumEt())
          if len(event.ZselJets)>0:
              fillJet(tr, 'Jet_leading', event.ZselJets[0])

          self.tree.tree.Fill()
       