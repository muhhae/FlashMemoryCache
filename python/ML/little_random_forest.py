import random_forest as rf
import common as c


def SetupModel():
    rf.SetupModel(5, 30, 20)


Train = c.Train
Test = c.Test
AddDatasets = c.AddDatasets
DescribeData = c.DescribeData
ExportONNX = c.ExportONNX
LoadONNX = c.LoadONNX
SaveModel = c.SaveModel
LoadModel = c.LoadModel
PlotSave = c.PlotSave
PlotShow = c.PlotShow
SetupData = c.SetupData
