from os import name
import re
from pathlib import Path
from typing import List

import pandas as pd
from common import OutputLog, extract_desc, ordinal


def GetModelResult(paths: List[str], included_sizes: List[str]):
    tmp = []
    for file in paths:
        if Path(file).stat().st_size == 0:
            continue
        prefix, desc = extract_desc(file)
        model = desc[-1]["model"]
        treshold = 0.5
        if "treshold" in desc[-1]:
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
                "Cache Size": float(desc[0]),
                "Ignore Obj Size": desc.count("ignore_obj_size"),
            }
        )
    return pd.DataFrame(tmp)


def GetOtherResult(paths: List[str], name: str):
    tmp = []
    for file in paths:
        if Path(file).stat().st_size == 0:
            continue
        prefix, desc = extract_desc(file)
        df = pd.read_csv(file)
        if df.empty:
            continue
        logs = [OutputLog(**row) for row in df.to_dict(orient="records")]
        tmp.append(
            {
                "Model": name,
                "Promotion": logs[0].n_promoted,
                "Miss Ratio": logs[0].miss_ratio,
                "Hit": logs[0].n_hit,
                "Miss": logs[0].n_miss,
                "Trace": prefix,
                "Cache Size": float(desc[0]),
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
            top_dist = float(desc[-1]["top"]) * 100
        if "treshold" in desc[-1]:
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


def GetBaseResult(paths: List[str]):
    tmp = []
    names = ["CLOCK", "Offline CLOCK"]
    for file in paths:
        if Path(file).stat().st_size == 0:
            continue
        prefix, desc = extract_desc(file)
        df = pd.read_csv(file)
        if df.empty:
            continue
        logs = [OutputLog(**row) for row in df.to_dict(orient="records")]
        for i, log in enumerate(logs):
            if i > 1:
                break
            tmp.append(
                {
                    "Model": names[i],
                    "Promotion": log.n_promoted,
                    "Miss Ratio": log.miss_ratio,
                    "Trace": prefix,
                    "Cache Size": float(desc[0]),
                    "Ignore Obj Size": desc.count("ignore_obj_size"),
                    "Hit": log.n_hit,
                    "Miss": log.n_miss,
                }
            )
    return pd.DataFrame(tmp)
