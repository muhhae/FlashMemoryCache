from sklearn.ensemble import RandomForestClassifier
import common as c

import var


def SetupModel(max_depth: int = 20, n_estimators: int = 100, parallelism: int = 4):
    if var.df is None:
        raise Exception("Datasets has not been set up")
    var.model = RandomForestClassifier(
        n_jobs=parallelism,
        n_estimators=n_estimators,
        max_depth=max_depth,
        max_features="sqrt",
        bootstrap=True,
        random_state=9,
    )


Train = c.Train
Test = c.Test
AddDatasets = c.AddDatasets
DescribeData = c.DescribeData
ExportONNX = c.ExportONNX
SaveModel = c.SaveModel
LoadModel = c.LoadModel
PlotSave = c.PlotSave
PlotShow = c.PlotShow
SetupData = c.SetupData
