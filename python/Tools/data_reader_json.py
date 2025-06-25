import json
import os
import re
from pathlib import Path
from pprint import pprint
from typing import List, cast

import pandas as pd
from common import OutputLog, extract_desc


def GetModelResult(paths: List[str], included_sizes: List[str]):
    tmp = []
    for file in paths:
        if Path(file).stat().st_size == 0:
            continue
        prefix, desc = extract_desc(file)
        model = ""
        if isinstance(desc[-1], dict):
            model = desc[-1]["model"]
        treshold = 0.5
        if "treshold" in desc[-1]:
            if isinstance(desc[-1], dict):
                treshold = desc[-1]["treshold"]
        size = model.split("_")[-1]
        if size not in included_sizes:
            continue
        size_pos = model.rfind("_")
        model = model[:size_pos]
        # model = f"{model}_{'spec' if size != 'All' else size}"
        model = f"{model}[cache_size={size},treshold={treshold}]"
        df = pd.read_csv(file)
        if df.empty:
            continue
        logs = [OutputLog(**row) for row in df.to_dict(orient="records")]
        tmp.append(
            {
                "Model": f"{model}",
                "Promotion": logs[0].n_promoted,
                "Miss Ratio": logs[0].miss_ratio,
                "Trace": prefix,
                "Cache Size": float(cast(str, desc[0])),
                "Ignore Obj Size": desc.count("ignore_obj_size"),
            }
        )
    return pd.DataFrame(tmp)


def ParseClassificationReport(report_string):
    overall = {}
    avg_specific = []
    class_specific = []
    report_start_index = report_string.find("Classification Report:")
    report_text = report_string[report_start_index:]
    lines = report_text.strip().split("\n")
    for line in lines:
        line = line.strip()
        if not line:
            continue
        match_overall_accuracy = re.match(r"^Accuracy:\s*([\d.]+)$", line)
        if match_overall_accuracy:
            overall["overall_accuracy"] = float(match_overall_accuracy.group(1))
            continue

        match_class = re.match(
            r"^(?P<label>(?!\baccuracy\b|\bmacro\b|\bweighted\b)\S+)\s+(?P<precision>[\d.]+)\s+(?P<recall>[\d.]+)\s+(?P<f1_score>[\d.]+)\s+(?P<support>[\d]+)$",
            line,
        )
        if match_class:
            data = match_class.groupdict()
            label = data["label"]
            class_specific.append(
                {
                    "class": label,
                    "precision": float(data["precision"]),
                    "recall": float(data["recall"]),
                    "f1-score": float(data["f1_score"]),
                    "support": int(data["support"]),
                }
            )
            continue

        match_table_accuracy = re.match(
            r"^accuracy\s+(?P<score>[\d.]+)\s+(?P<support>[\d]+)$", line
        )
        if match_table_accuracy:
            data = match_table_accuracy.groupdict()
            overall["accuracy_score"] = float(data["score"])
            overall["accuracy_support"] = int(data["support"])
            continue

        match_avg = re.match(
            r"^(?P<avg_type>macro avg|weighted avg)\s+(?P<precision>[\d.]+)\s+(?P<recall>[\d.]+)\s+(?P<f1_score>[\d.]+)\s+(?P<support>[\d]+)$",
            line,
        )
        if match_avg:
            data = match_avg.groupdict()
            avg_type_key = data["avg_type"].replace("avg", "").strip()
            avg_specific.append(
                {
                    "type": avg_type_key,
                    "precision": float(data["precision"]),
                    "recall": float(data["recall"]),
                    "f1-score": float(data["f1_score"]),
                    "support": int(data["support"]),
                }
            )
            continue
    return (
        pd.DataFrame([overall]),
        pd.DataFrame(avg_specific),
        pd.DataFrame(class_specific),
    )


def GetModelMetrics(
    paths: List[str],
    included_sizes: List[str],
    included_treshold: List[str],
):
    tmp = []
    for p in paths:
        f = open(p, "r")
        content = f.read()
        report = content[
            content.find("Classification Report")
            + len("Classification Report:\n") : content.find("Confusion Matrix") - 1
        ]
        overall, avg, class_specific = ParseClassificationReport(content)
        kw = "Confusion Matrix"

        content = content[content.find(kw) + len(kw) :]
        content = content[: content.find(kw)]
        content = content.replace(":", "").strip()
        model = p.replace(".md", "").replace(".txt", "")
        model = Path(p).stem
        model, desc = extract_desc(model)
        size = desc[0]
        if size not in included_sizes:
            continue
        top_dist = -1
        treshold = 0.5
        if "top" in desc[-1]:
            if isinstance(desc[-1], dict):
                top_dist = float(desc[-1]["top"]) * 100
        if "treshold" in desc[-1]:
            if isinstance(desc[-1], dict):
                treshold = float(desc[-1]["treshold"])
        if treshold not in included_treshold:
            continue
        # model = f"{model}_{'spec' if size != 'All' else size}"
        model = f"{model}"

        tmp.append(
            {
                "Treshold": treshold,
                "Model": model,
                "Cache Size": size,
                "Report": report,
                "Top (%)": top_dist,
            }
        )
    return pd.DataFrame(tmp)


def ProcessResultJSON(result: dict, file):
    prefix, desc = extract_desc(file)
    metrics = result["metrics"]
    dram = metrics[0]
    flash = metrics[1]
    return {
        "Flash Admission Treshold": flash["admission_treshold"],
        "DRAM Algorithm": dram["algorithm"],
        "Algorithm": flash["algorithm"],
        "Inserted": flash["inserted"],
        "Reinserted": flash["reinserted"],
        "Write": flash["reinserted"] + flash["inserted"],
        "Flash Miss Ratio": flash["miss_ratio"],
        "DRAM Miss Ratio": dram["miss_ratio"],
        "Overall Miss Ratio": result["miss_ratio"],
        "Flash Hit": flash["hit"],
        "DRAM Hit": dram["hit"],
        "Overall Hit": result["hit"],
        "Flash Request": flash["req"],
        "DRAM Request": dram["req"],
        "Overall Request": result["req"],
        "Trace": os.path.basename(prefix),
        "JSON File": os.path.basename(file),
        "Cache Size": float(cast(str, desc[0])),
        "Ignore Obj Size": desc.count("ignore_obj_size"),
    }


def GetOfflineClockResult(paths: List[str]):
    tmp = []
    names = ["CLOCK", "Offline CLOCK"]
    for file in paths:
        if Path(file).stat().st_size == 0:
            continue
        f = open(file, "r")
        j = json.load(f)
        f.close()
        for i, result in enumerate(j["results"]):
            if i > 1:
                break
            j = ProcessResultJSON(result, file)
            if j["Algorithm"] != "offline-clock":
                continue
            j["Algorithm"] = names[i]
            tmp.append(j)
    return pd.DataFrame(tmp)


def GetOtherResult(paths: List[str], plot_name: str, json_name: str):
    tmp = []
    for file in paths:
        if Path(file).stat().st_size == 0:
            continue
        f = open(file, "r")
        j = json.load(f)
        f.close()
        r = ProcessResultJSON(j["results"][0], file)
        if r["Algorithm"] != json_name:
            continue
        r["Algorithm"] = plot_name
        tmp.append(r)
    return pd.DataFrame(tmp)
