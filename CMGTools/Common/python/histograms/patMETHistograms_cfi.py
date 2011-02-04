import FWCore.ParameterSet.Config as cms

patMETHistograms = cms.EDAnalyzer(
    "GenericPatMETHistograms",
    inputCollection = cms.InputTag("patMETsPFlow"),
    histograms = cms.PSet(
        met = cms.VPSet(
            cms.PSet( 
               var = cms.string('pt()'),
               nbins = cms.int32(100),
               low = cms.double(0),
               high = cms.double(1000)
               )
            ),
    
        sumEt = cms.VPSet(
            cms.PSet( 
               var = cms.string('sumEt()'),
               nbins = cms.int32(100),
               low = cms.double(0),
               high = cms.double(2000)
               )
            ),
        
        phi = cms.VPSet(
            cms.PSet( 
               var = cms.string('phi()'),
               nbins = cms.int32(100),
               low = cms.double(-3.15),
               high = cms.double(3.15)
               )
            ),
        )
    
    )
