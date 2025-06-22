import glob
import multiprocessing
import os
import typing as T
from collections import defaultdict
from pathlib import Path

import pandas as pd
import plotly.io as pio
import tabulate as tb

from common import extract_desc, sort_key
from data_reader import GetBaseResult, GetModelMetrics, GetModelResult, GetOtherResult
from docs_writer import Write, WriteFig, WriteHTML
from plotly_wrapper import Scatter, Box

pd.set_option("display.max_rows", None)

urls = []
with open("../../trace/reasonable_traces.txt") as f:
    urls = [line.strip() for line in f if line.strip()]


def WriteModelSummaries(md, html, base_result, models_result, included_sizes):
    tmp = []
    for model in models_result["Model"].unique():
        current = models_result.query("Model == @model")
        for i, x in current.iterrows():
            base_log = base_result.query(
                "Trace == @x['Trace'] and `Cache Size` == @x['Cache Size'] and Model == 'Offline Clock 1st iteration' and `Ignore Obj Size` == @x['Ignore Obj Size']"
            )
            tmp.append(
                {
                    "Model": model,
                    "Trace": x["Trace"],
                    "Cache Size": x["Cache Size"],
                    "Miss Ratio Reduced (%)": (
                        base_log["Miss Ratio"].item() - x["Miss Ratio"]
                    )
                    / base_log["Miss Ratio"].item()
                    * 100,
                    "Promotion Reduced (%)": (
                        base_log["Promotion"].item() - x["Promotion"]
                    )
                    / base_log["Promotion"].item()
                    * 100,
                    "Better Than Base": x["Miss Ratio"] < base_log["Miss Ratio"].item(),
                }
            )
    bt_data = defaultdict(list)
    mr_data = defaultdict(list)
    p_data = defaultdict(list)
    data = pd.DataFrame(tmp)
    for model in data["Model"].unique():
        current = data.query("Model == @model")
        bt_data["Model"].append(model)
        bt_data["Better than base % of the times"].append(
            (current["Better Than Base"].value_counts(normalize=True).get(True) or 0)
            * 100
        )

        mr_data["Model"].append(model)
        mr_data["Max"].append(current["Miss Ratio Reduced (%)"].max())
        mr_data["Min"].append(current["Miss Ratio Reduced (%)"].min())
        mr_data["Avg"].append(current["Miss Ratio Reduced (%)"].mean())
        mr_data["Mdn"].append(current["Miss Ratio Reduced (%)"].median())

        p_data["Model"].append(model)
        p_data["Max"].append(current["Promotion Reduced (%)"].max())
        p_data["Min"].append(current["Promotion Reduced (%)"].min())
        p_data["Avg"].append(current["Promotion Reduced (%)"].mean())
        p_data["Mdn"].append(current["Promotion Reduced (%)"].median())

    Write(md, html, "# Model Summaries  \n")
    Write(md, html, tb.tabulate(bt_data, headers="keys", tablefmt="html") + "  \n\n")
    Write(md, html, "## Promotion Reduced (%)  \n")
    Write(md, html, tb.tabulate(p_data, headers="keys", tablefmt="html") + "  \n\n")
    Write(md, html, "## Miss Ratio Reduced (%)  \n")
    Write(md, html, tb.tabulate(mr_data, headers="keys", tablefmt="html") + "  \n\n")
    Write(md, html, "# Model Summaries Plot  \n")

    for title in ["Miss Ratio Reduced (%)", "Promotion Reduced (%)"]:
        fig = Box(
            data,
            x=title,
            y="Model",
            title=title,
            color="Model",
        )
        Write(md, html, f"## {title}\n")
        WriteFig(md, html, fig)

    symbol_map = {
        "Offline Clock 1st iteration": "square-dot",
        "Offline Clock 2nd iteration": "diamond-dot",
        "Zipf Optimal Distribution": "x-dot",
    }
    for model in data["Model"].unique():
        if model not in symbol_map:
            symbol_map[model] = "circle"

    Write(md, html, "# Mean Promotion vs Miss Ratio  \n")
    Write(md, html, "## Cache Size All  \n")
    df = (
        data.groupby("Model")[["Promotion Reduced (%)", "Miss Ratio Reduced (%)"]]
        .mean()
        .reset_index()
        .sort_values(by="Miss Ratio Reduced (%)", ascending=False)
    )

    fig = Scatter(
        df,
        symbol="Model",
        symbol_map=symbol_map,
        x="Promotion Reduced (%)",
        y="Miss Ratio Reduced (%)",
        color="Model",
    )
    WriteFig(md, html, fig)
    headers = df.columns.tolist()
    table_data = df.values.tolist()
    Write(
        md,
        html,
        f"{tb.tabulate(table_data, headers=headers, tablefmt='html')}  \n\n",
    )
    for size in data["Cache Size"].unique():
        if str(size) not in included_sizes:
            continue
        tmp = data[data["Cache Size"] == size]
        df = (
            tmp.groupby("Model")[["Promotion Reduced (%)", "Miss Ratio Reduced (%)"]]
            .mean()
            .reset_index()
            .sort_values(by="Miss Ratio Reduced (%)", ascending=False)
        )
        Write(md, html, f"## Cache Size {size}  \n")
        fig = Scatter(
            df,
            symbol="Model",
            symbol_map=symbol_map,
            x="Promotion Reduced (%)",
            y="Miss Ratio Reduced (%)",
            color="Model",
        )
        WriteFig(md, html, fig)

        headers = df.columns.tolist()
        table_data = df.values.tolist()

        Write(
            md,
            html,
            f"{tb.tabulate(table_data, headers=headers, tablefmt='html')}  \n\n",
        )

    Write(md, html, "# Median Promotion vs Miss Ratio  \n")
    Write(md, html, "## Cache Size All  \n")
    df = (
        data.groupby("Model")[["Promotion Reduced (%)", "Miss Ratio Reduced (%)"]]
        .median()
        .reset_index()
        .sort_values(by="Miss Ratio Reduced (%)", ascending=False)
    )
    fig = Scatter(
        df,
        symbol="Model",
        symbol_map=symbol_map,
        x="Promotion Reduced (%)",
        y="Miss Ratio Reduced (%)",
        color="Model",
    )
    WriteFig(md, html, fig)

    headers = df.columns.tolist()
    table_data = df.values.tolist()

    Write(
        md,
        html,
        f"{tb.tabulate(table_data, headers=headers, tablefmt='html')}  \n\n",
    )

    for size in data["Cache Size"].unique():
        if str(size) not in included_sizes:
            continue
        tmp = data[data["Cache Size"] == size]
        df = (
            tmp.groupby("Model")[["Promotion Reduced (%)", "Miss Ratio Reduced (%)"]]
            .median()
            .reset_index()
            .sort_values(by="Miss Ratio Reduced (%)", ascending=False)
        )
        Write(md, html, f"## Cache Size {size}  \n")
        fig = Scatter(
            df,
            symbol="Model",
            symbol_map=symbol_map,
            x="Promotion Reduced (%)",
            y="Miss Ratio Reduced (%)",
            color="Model",
        )
        WriteFig(md, html, fig)
        headers = df.columns.tolist()
        table_data = df.values.tolist()
        Write(
            md,
            html,
            f"\n{tb.tabulate(table_data, headers=headers, tablefmt='html')}  \n\n",
        )


def WriteModelMetrics(md, html, model_metrics: pd.DataFrame):
    if model_metrics.empty:
        print("Empty Model Metrics")
        return
    Write(md, html, "# Model Classification Report  \n")
    for m in model_metrics["Model"].unique():
        model_filtered = model_metrics.query("Model == @m").sort_values(by="Cache Size")
        Write(md, html, f"## {m}  \n")
        for size in model_filtered["Cache Size"].unique():
            size_filtered = model_filtered.query(
                "`Cache Size` == @size",
            ).sort_values(by="Treshold")
            Write(md, html, f"### {size}  \n")
            for treshold in size_filtered["Treshold"].unique():
                treshold_filtered = size_filtered.query(
                    "Treshold == @treshold"
                ).sort_values(by="Top (%)")
                Write(md, html, f"#### Treshold: {treshold}  \n")
                prev = 0
                for top in treshold_filtered["Top (%)"].unique():
                    if top != -1:
                        Write(md, html, f"##### {prev:g}-{top:g}%  \n")
                        prev = top
                    else:
                        Write(md, html, "##### All  \n")
                    top_filtered = treshold_filtered.query("`Top (%)` == @top")
                    report = top_filtered["Report"].tolist()
                    for r in report:
                        Write(md, html, f"```\n{r}\n```  \n")


def WriteIndividualResult(md, html, results, included_sizes):
    Write(md, html, "# Individual Workload Result  \n")
    df = pd.concat(results, ignore_index=True)

    ignores = sorted(df["Ignore Obj Size"].unique())
    traces = df["Trace"].unique()
    sizes = df["Cache Size"].unique()

    symbol_map = {
        "Offline Clock 1st iteration": "square-dot",
        "Offline Clock 2nd iteration": "diamond-dot",
        "Zipf Optimal Distribution": "x-dot",
    }
    for model in df["Model"].unique():
        if model not in symbol_map:
            symbol_map[model] = "circle"

    for trace in traces:
        df_trace = df.query("`Trace` == @trace")
        Write(md, html, f"## {Path(trace).stem}  \n")
        for ignore in ignores:
            df_ignore = df_trace.query("`Ignore Obj Size` == @ignore")
            if ignore:
                Write(md, html, "## Ignore Obj Size  \n")
            for size in sizes:
                if str(size) not in included_sizes:
                    continue
                df_size = df_ignore.query("`Cache Size` == @size")
                Write(md, html, f"### {size}  \n")
                fig = Scatter(
                    df_size,
                    symbol="Model",
                    symbol_map=symbol_map,
                    x="Promotion",
                    y="Miss Ratio",
                    color="Model",
                )
                WriteFig(md, html, fig)
                headers = df.columns.tolist()
                table_data = df_size.sort_values(by="Miss Ratio").values.tolist()
                Write(
                    md,
                    html,
                    f"{tb.tabulate(table_data, headers=headers, tablefmt='html')}  \n\n",
                )


def Analyze(
    paths: T.List[str],
    output_path: str,
    html_path: str,
    Title: str,
    models_metrics_paths: T.List[str],
    included_models: T.List[str],
    included_treshold: T.List[str],
    included_sizes: T.List[str],
):
    print(f"Analyzing for {Title}")

    model_paths = [f for f in paths if "ML" in f]
    lru_paths = [f for f in paths if "lru" in f]
    dist_optimal_paths = [f for f in paths if "dist_optimal" in f]
    base_paths = [
        f
        for f in paths
        if f not in set(model_paths) | set(lru_paths) | set(dist_optimal_paths)
    ]

    model_paths = [
        f
        for f in model_paths
        if (
            (model := extract_desc(f)[1][-1]["model"])[: model.rfind("_")]
            in included_models
            and (
                "treshold" not in extract_desc(f)[1][-1]
                or float(extract_desc(f)[1][-1]["treshold"]) in included_treshold
            )
        )
    ]
    models_metrics_paths = [
        f
        for f in models_metrics_paths
        if (p := Path(f).stem)[: p.rfind("[")] in included_models
    ]
    if len(model_paths) == 0 or len(models_metrics_paths) == 0:
        print(f"Empty data for {Title}")
        return

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    os.makedirs(os.path.dirname(html_path), exist_ok=True)

    md = open(output_path, "w")
    html = open(Path(html_path), "w")
    Write(md, html, f"# {Title}  \n# Result  \n")

    model_metrics = GetModelMetrics(
        models_metrics_paths, included_sizes, included_treshold
    )
    base_result = GetBaseResult(base_paths)
    model_result = GetModelResult(model_paths, included_sizes)
    lru_result = GetOtherResult(lru_paths, "LRU")
    dist_optimal_result = GetOtherResult(
        dist_optimal_paths, "Zipf Optimal Distribution"
    )

    WriteModelSummaries(
        md,
        html,
        base_result,
        pd.concat(
            [model_result, base_result, dist_optimal_result],
        ),
        included_sizes,
    )
    WriteModelMetrics(md, html, model_metrics)
    WriteIndividualResult(
        md,
        html,
        [base_result, model_result, dist_optimal_result],
        included_sizes,
    )
    WriteHTML(html)
    print(f"Finished analyzing for {Title}")


def Summarize(
    additional_desc: str,
    title: str,
    included_models: T.List[str],
    included_treshold: T.List[str],
    included_sizes: T.List[str],
    result_dir: str,
):
    files = sorted(glob.glob(os.path.join(result_dir, "*.csv")), key=sort_key)
    files = [f for f in files if f.count(additional_desc)]

    models_metric_files = glob.glob("../ML/model/*.md") + glob.glob("../ML/model/*.txt")
    models_metric_files = [m for m in models_metric_files if m.count(additional_desc)]
    models_metric_files = sorted(models_metric_files, key=sort_key)

    model_metrics: T.Dict[bool, T.List[str]] = {}
    model_metrics[True] = [f for f in models_metric_files if f.count("ignore_obj_size")]
    model_metrics[False] = [
        f for f in models_metric_files if not f.count("ignore_obj_size")
    ]

    test_prefix = [extract_desc(f)[0] for f in files if "test" in f]
    files = [f for f in files if extract_desc(f)[0] in test_prefix]

    # [ignore_obj_size] -> paths
    paths: T.Dict[bool, T.List[str]] = {
        False: [f for f in files if "ignore_obj_size" not in f],
        True: [f for f in files if "ignore_obj_size" in f],
    }

    # Analyze(
    #     paths[0],
    #     f"../../result/{title}_obj_size_not_ignored.md",
    #     f"../../docs/{title}_obj_size_not_ignored.html",
    #     f"{title} Test Data Result Obj Size Not Ignored",
    #     model_metrics[0],
    #     included_models,
    #     included_treshold,
    #     included_sizes,
    # )
    Analyze(
        paths[True],
        f"../../result/{title}_obj_size_ignored.md",
        f"../../docs/{title}_obj_size_ignored.html",
        f"{title} Test Data Result Obj Size Ignored",
        model_metrics[True],
        included_models,
        included_treshold,
        included_sizes,
    )
    # Analyze(
    #     files,
    #     f"../../result/{title}.md",
    #     f"../../docs/{title}.html",
    #     f"{title} Test Data Result Combined",
    #     model_metrics[0] + model_metrics[1],
    #     included_models,
    #     included_treshold,
    #     included_sizes,
    # )


ALL_MODELS = [
    "little_random_forest",
    "logistic_regression",
    "logistic_regression_v2",
    "logistic_regression_v3",
    "logistic_regression_v4",
    "LR_1",
    "LR_1_std_scaler",
    "LR_1_robust_scaler",
    "LR_1_log",
    "LR_1_mean",
    "LR_2",
    "LR_2_log",
    "LR_2_mean",
    "LR_3",
    "LR_3_log",
    "LR_3_mean",
    "LR_4",
    "LR_4_std_scaler",
    "LR_4_robust_scaler",
    "LR_4_log",
    "LR_4_mean",
    "LR_5",
    "LR_5_imba",
    "LR_6",
    "LR_6_imba",
    "LR_7",
    "LR_8",
    "LR_9",
    "LR_7_decay_rtime",
    "LR_7_decay_vtime",
    "LR_8_decay_rtime",
    "LR_8_decay_vtime",
    "LR_9_decay_rtime",
    "LR_9_decay_vtime",
    "LR_decay_rtime",
    "LR_decay_vtime",
    "LR_7_w_0_5",
    "LR_7_decay_rtime_w_0_5",
    "LR_7_decay_vtime_w_0_5",
    "LR_8_decay_rtime_w_0_5",
    "LR_8_decay_vtime_w_0_5",
    "LR_9_decay_rtime_w_0_5",
    "LR_9_decay_vtime_w_0_5",
    "LR_decay_vtime_w_0_5",
    "LR_decay_rtime_w_0_5",
    "LR_7_w_0_75",
    "LR_7_decay_rtime_w_0_75",
    "LR_7_decay_vtime_w_0_75",
    "LR_8_decay_rtime_w_0_75",
    "LR_8_decay_vtime_w_0_75",
    "LR_9_decay_rtime_w_0_75",
    "LR_9_decay_vtime_w_0_75",
    "LR_decay_vtime_w_0_75",
    "LR_decay_rtime_w_0_75",
    "LR_id",
    "LR_7_id",
    "LR_8_id",
    "LR_9_id",
]
BASE_MODELS = [
    "LR_1",
    "LR_2",
    "LR_3",
    "LR_4",
    "LR_5",
    "LR_6",
    "LR_7",
    "LR_8",
    "LR_9",
]
ALL_TRESHOLD = [
    0.3,
    0.5,
    0.6,
    0.7,
    0.8,
    0.9,
]

BEST_MODELS = [
    "LR_7",
    "LR_7_w_0_75",
    "LR_7_w_0_5",
]

BEST_TRESHOLD = [
    0.6,
    0.7,
    0.8,
]
RESULT_DIR = "../../result/log"


def main():
    summarize_calls_args = []
    for trace, title in [("zipf1", "Zipf1"), ("cloudphysics", "CloudPhysics")]:
        summarize_calls_args += [
            (
                trace,
                f"{title} Current Best Models",
                BEST_MODELS,
                BEST_TRESHOLD,
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                f"{title} Models that use ObjID",
                [
                    "LR_id",
                    "LR_7_id",
                    "LR_8_id",
                    "LR_9_id",
                ],
                ALL_TRESHOLD,
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                f"{title} Models that use ObjID vs Model that does not",
                [
                    "LR_id",
                    "LR_7_id",
                    "LR_8_id",
                    "LR_9_id",
                    "LR_7",
                    "LR_8",
                    "LR_9",
                ],
                ALL_TRESHOLD,
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                title,
                ["LR_1", "LR_5_imba"],
                ALL_TRESHOLD,
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                f"{title} All Model with base treshold",
                ALL_MODELS,
                [0.5],
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                f"{title} All Model with all treshold",
                ALL_MODELS,
                ALL_TRESHOLD,
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                f"{title} New Model",
                ["LR_7", "LR_8", "LR_9"],
                ALL_TRESHOLD,
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                f"{title} New Model With Selected Treshold",
                ["LR_7", "LR_8", "LR_9"],
                [0.8, 0.9],
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                f"{title} Base Model",
                BASE_MODELS,
                [0.5],
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                f"{title} Decay Model with All Treshold ",
                [
                    "LR_7_decay_rtime",
                    "LR_7_decay_vtime",
                    "LR_8_decay_rtime",
                    "LR_8_decay_vtime",
                    "LR_9_decay_rtime",
                    "LR_9_decay_vtime",
                    "LR_decay_rtime",
                    "LR_decay_vtime",
                ],
                ALL_TRESHOLD,
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                f"{title} Selected Non-Decay vs Decay Model with All Treshold",
                [
                    "LR_7",
                    "LR_7_decay_rtime",
                    "LR_7_decay_vtime",
                ],
                ALL_TRESHOLD,
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
            (
                trace,
                f"{title} Weighted Models with All Treshold",
                [
                    "LR_7",
                    "LR_8",
                    "LR_9",
                    "LR_7_decay_rtime",
                    "LR_7_decay_vtime",
                    "LR_8_decay_rtime",
                    "LR_8_decay_vtime",
                    "LR_9_decay_rtime",
                    "LR_9_decay_vtime",
                    "LR_decay_rtime",
                    "LR_decay_vtime",
                    "LR_7_w_0_5",
                    "LR_7_decay_rtime_w_0_5",
                    "LR_7_decay_vtime_w_0_5",
                    "LR_8_decay_rtime_w_0_5",
                    "LR_8_decay_vtime_w_0_5",
                    "LR_9_decay_rtime_w_0_5",
                    "LR_9_decay_vtime_w_0_5",
                    "LR_decay_vtime_w_0_5",
                    "LR_decay_rtime_w_0_5",
                    "LR_7_w_0_75",
                    "LR_7_decay_rtime_w_0_75",
                    "LR_7_decay_vtime_w_0_75",
                    "LR_8_decay_rtime_w_0_75",
                    "LR_8_decay_vtime_w_0_75",
                    "LR_9_decay_rtime_w_0_75",
                    "LR_9_decay_vtime_w_0_75",
                    "LR_decay_vtime_w_0_75",
                    "LR_decay_rtime_w_0_75",
                ],
                ALL_TRESHOLD,
                [
                    "0.01",
                    "0.1",
                    "0.2",
                    "0.4",
                ],
                RESULT_DIR,
            ),
        ]
        for treshold in ALL_TRESHOLD:
            summarize_calls_args += [
                (
                    trace,
                    f"{title} Decay Model with Treshold {treshold}",
                    [
                        "LR_7_decay_rtime",
                        "LR_7_decay_vtime",
                        "LR_8_decay_rtime",
                        "LR_8_decay_vtime",
                        "LR_9_decay_rtime",
                        "LR_9_decay_vtime",
                        "LR_decay_rtime",
                        "LR_decay_vtime",
                        "LR_7_decay_rtime_w_0_5",
                        "LR_7_decay_vtime_w_0_5",
                        "LR_8_decay_rtime_w_0_5",
                        "LR_8_decay_vtime_w_0_5",
                        "LR_9_decay_rtime_w_0_5",
                        "LR_9_decay_vtime_w_0_5",
                        "LR_decay_vtime_w_0_5",
                        "LR_decay_rtime_w_0_5",
                        "LR_7_decay_rtime_w_0_75",
                        "LR_7_decay_vtime_w_0_75",
                        "LR_8_decay_rtime_w_0_75",
                        "LR_8_decay_vtime_w_0_75",
                        "LR_9_decay_rtime_w_0_75",
                        "LR_9_decay_vtime_w_0_75",
                        "LR_decay_vtime_w_0_75",
                        "LR_decay_rtime_w_0_75",
                    ],
                    [treshold],
                    [
                        "0.01",
                        "0.1",
                        "0.2",
                        "0.4",
                    ],
                    RESULT_DIR,
                ),
                (
                    trace,
                    f"{title} Selected Non-Decay vs Decay Model with Treshold {treshold}",
                    [
                        "LR_7",
                        "LR_7_decay_rtime",
                        "LR_7_decay_vtime",
                        "LR_decay_vtime",
                        "LR_decay_rtime",
                    ],
                    [treshold],
                    [
                        "0.01",
                        "0.1",
                        "0.2",
                        "0.4",
                    ],
                    RESULT_DIR,
                ),
                (
                    trace,
                    f"{title} Weighted Models with Treshold {treshold}",
                    [
                        "LR_7",
                        "LR_8",
                        "LR_9",
                        "LR_7_decay_rtime",
                        "LR_7_decay_vtime",
                        "LR_8_decay_rtime",
                        "LR_8_decay_vtime",
                        "LR_9_decay_rtime",
                        "LR_9_decay_vtime",
                        "LR_decay_rtime",
                        "LR_decay_vtime",
                        "LR_7_w_0_5",
                        "LR_7_decay_rtime_w_0_5",
                        "LR_7_decay_vtime_w_0_5",
                        "LR_8_decay_rtime_w_0_5",
                        "LR_8_decay_vtime_w_0_5",
                        "LR_9_decay_rtime_w_0_5",
                        "LR_9_decay_vtime_w_0_5",
                        "LR_decay_vtime_w_0_5",
                        "LR_decay_rtime_w_0_5",
                        "LR_7_w_0_75",
                        "LR_7_decay_rtime_w_0_75",
                        "LR_7_decay_vtime_w_0_75",
                        "LR_8_decay_rtime_w_0_75",
                        "LR_8_decay_vtime_w_0_75",
                        "LR_9_decay_rtime_w_0_75",
                        "LR_9_decay_vtime_w_0_75",
                        "LR_decay_vtime_w_0_75",
                        "LR_decay_rtime_w_0_75",
                    ],
                    [treshold],
                    [
                        "0.01",
                        "0.1",
                        "0.2",
                        "0.4",
                    ],
                    RESULT_DIR,
                ),
            ]
    with multiprocessing.Pool(processes=10) as pool:
        pool.starmap(Summarize, summarize_calls_args)


if __name__ == "__main__":
    main()
