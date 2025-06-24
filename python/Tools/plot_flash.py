import os
import pickle
from glob import glob
from pathlib import Path
from pprint import pprint

import pandas as pd
from common import CalculateReduction, sort_key
from data_reader_json import GetOfflineClockResult, GetOtherResult
from docs_writer import Write, WriteFig, WriteHTML
from plotly.graph_objs import Figure
from plotly_wrapper import Scatter, VerticalCompositionBar
from tabulate import tabulate


def CreateFlashWriteComposition(df: pd.DataFrame) -> Figure:
    return VerticalCompositionBar(
        df,
        X="Model",
        Ys=[
            ("Miss", "Cache Miss"),
            ("Promotion", "Reinsertion"),
        ],
        title="Flash Write (Reinsertion + Miss) by Algorithm",
        yaxis_title="Flash Write",
        xaxis_title="Algorithm",
        mode="stack",
    )


def WriteMean(md, html, df: pd.DataFrame):
    Write(md, html, "# Mean  \n")
    for s in df["Cache Size"].unique():
        Write(md, html, f"## {s}  \n")
        data = (
            df.query("`Cache Size` == @s")
            .groupby("Model")[["Miss Ratio", "Flash Write", "Promotion", "Miss"]]
            .mean()
            .reset_index()
            .sort_values(by="Flash Write")
        )
        fig = Scatter(
            data,
            x="Flash Write",
            y="Miss Ratio",
            color="Model",
            symbol="Model",
        )
        WriteFig(md, html, fig)
        fig = CreateFlashWriteComposition(data)
        WriteFig(md, html, fig)
        Write(
            md,
            html,
            tabulate(
                data[["Model", "Miss Ratio", "Miss", "Promotion", "Flash Write"]],
                headers=[
                    "Algorithm",
                    "Miss Ratio",
                    "Cache Miss",
                    "Reinsertion",
                    "Flash Write",
                ],
                tablefmt="html",
                showindex="never",
                intfmt=",",
            )
            + "  \n\n",
        )


def WriteMeanReduction(md, html, df: pd.DataFrame):
    Write(md, html, "# Mean Reduction Compared to FIFO  \n")
    for s in df["Cache Size"].unique():
        Write(md, html, f"## {s}  \n")
        data = (
            df.query("`Cache Size` == @s")
            .groupby("Model")[
                [
                    "Miss Ratio Reduction",
                    "Flash Write Reduction",
                    "Miss Reduction",
                    "Promotion Reduction",
                ]
            ]
            .mean()
            .reset_index()
            .sort_values(by="Flash Write Reduction", ascending=False)
        )

        fig = Scatter(
            data,
            x="Flash Write Reduction",
            y="Miss Ratio Reduction",
            color="Model",
            symbol="Model",
        )
        WriteFig(md, html, fig)
        Write(
            md,
            html,
            tabulate(
                data[
                    [
                        "Model",
                        "Miss Ratio Reduction",
                        "Miss Reduction",
                        "Promotion Reduction",
                        "Flash Write Reduction",
                    ]
                ],
                headers=[
                    "Algorithm",
                    "Miss Ratio Reduction",
                    "Cache Miss Reduction",
                    "Reinsertion Reduction",
                    "Flash Write Reduction",
                ],
                tablefmt="html",
                showindex="never",
                intfmt=",",
            )
            + "  \n\n",
        )


def WriteIndividual(md, html, add_desc: str, df: pd.DataFrame):
    Write(md, html, f"# Individual Result {add_desc} \n")
    for s in sorted(df["Cache Size"].unique()):
        Write(md, html, f"## {s}  \n")
        for t in df["Trace"].unique():
            Write(md, html, f"### {t}  \n")
            data = df.query("`Cache Size` == @s and `Trace` == @t").sort_values(
                by="Write"
            )
            WriteFig(
                md,
                html,
                Scatter(
                    data,
                    x="Write",
                    y="Overall Miss Ratio",
                    color="Algorithm",
                    symbol="Algorithm",
                ),
            )
            WriteFig(
                md,
                html,
                Scatter(
                    data,
                    x="Write",
                    y="Flash Miss Ratio",
                    color="Algorithm",
                    symbol="Algorithm",
                ),
            )
            WriteFig(
                md,
                html,
                VerticalCompositionBar(
                    data,
                    X="Algorithm",
                    Ys=[
                        "Inserted",
                        "Reinserted",
                    ],
                    title="Flash Write (Inserted + Reinserted) by Algorithm",
                    yaxis_title="Flash Write",
                    xaxis_title="Algorithm",
                    mode="stack",
                ),
            )
            Write(
                md,
                html,
                tabulate(
                    data[
                        [
                            "Algorithm",
                            "Flash Admission Treshold",
                            "Overall Request",
                            "Flash Request",
                            "DRAM Request",
                            "Overall Miss Ratio",
                            "Flash Miss Ratio",
                            "DRAM Miss Ratio",
                            "Overall Hit",
                            "Flash Hit",
                            "DRAM Hit",
                            "Write",
                            "JSON File",
                        ]
                    ],
                    headers="keys",
                    tablefmt="html",
                    showindex="never",
                    intfmt=",",
                )
                + "  \n\n",
            )


def WriteIndividualReduction(md, html, df: pd.DataFrame):
    Write(md, html, "# Individual Reduction Result  \n")
    for s in df["Cache Size"].unique():
        Write(md, html, f"## {s}  \n")
        for t in df["Trace"].unique():
            Write(md, html, f"### {Path(t).stem}  \n")
            data = df.query("`Cache Size` == @s and `Trace` == @t").sort_values(
                by="Flash Write Reduction", ascending=False
            )
            fig = Scatter(
                data,
                x="Flash Write Reduction",
                y="Miss Ratio Reduction",
                color="Model",
                symbol="Model",
            )
            WriteFig(md, html, fig)
            Write(
                md,
                html,
                tabulate(
                    data[
                        [
                            "Model",
                            "Miss Ratio Reduction",
                            "Miss Reduction",
                            "Promotion Reduction",
                            "Flash Write Reduction",
                            "JSON File",
                        ]
                    ],
                    headers=[
                        "Algorithm",
                        "Miss Ratio Reduction",
                        "Cache Miss Reduction",
                        "Reinsertion Reduction",
                        "Flash Write Reduction",
                    ],
                    tablefmt="html",
                    showindex="never",
                    intfmt=",",
                )
                + "  \n\n",
            )


def Sumz(files: list[str], title: str, ignore_obj_size: bool = True, use_cache=True):
    files = [f for f in files if ("ignore_obj_size" in f) == ignore_obj_size]
    combined: pd.DataFrame
    cache = f".cache/{title}.pkl"
    os.makedirs(".cache", exist_ok=True)
    if use_cache and Path(cache).exists():
        print("Using cached DataFrame")
        with open(cache, "rb") as c:
            combined = pickle.load(c)
    else:
        print("Processing DataFrame")
        offline_clock = GetOfflineClockResult(
            [f for f in files if "offline-clock" in f]
        )
        fifo = GetOtherResult(files, "fifo", "FIFO")
        lru = GetOtherResult(files, "lru", "LRU")
        combined = pd.concat([offline_clock, fifo, lru])
        # pprint(combined)
        # combined = (
        #     combined.groupby(["Ignore Obj Size", "Trace", "Cache Size"])
        #     .apply(CalculateReduction, "FIFO", "Flash Write", include_groups=True)
        #     .reset_index(drop=True)
        # )
        # combined = (
        #     combined.groupby(["Ignore Obj Size", "Trace", "Cache Size"])
        #     .apply(CalculateReduction, "FIFO", "Miss Ratio", include_groups=True)
        #     .reset_index(drop=True)
        # )
        # combined = (
        #     combined.groupby(["Ignore Obj Size", "Trace", "Cache Size"])
        #     .apply(CalculateReduction, "FIFO", "Promotion", include_groups=True)
        #     .reset_index(drop=True)
        # )
        # combined = (
        #     combined.groupby(["Ignore Obj Size", "Trace", "Cache Size"])
        #     .apply(CalculateReduction, "FIFO", "Miss", include_groups=True)
        #     .reset_index(drop=True)
        # )

        with open(cache, "wb") as c:
            pickle.dump(combined, c)

    pprint(combined)
    pprint(combined.columns)

    os.makedirs("../../docs/", exist_ok=True)
    os.makedirs("../../results/", exist_ok=True)

    html = open(f"../../docs/{title}.html", "w")
    md = open(f"../../results/{title}.md", "w")

    # WriteMean(md, html, combined)
    for t in sorted(combined["Flash Admission Treshold"].unique()):
        WriteIndividual(
            md,
            html,
            f"Flash Admission Treshold: {t}",
            combined.query("`Flash Admission Treshold` == @t"),
        )

    # WriteMeanReduction(md, html, combined)
    # WriteIndividualReduction(md, html, combined)

    WriteHTML(html)


def main():
    log_path = "../../results/log/"
    files = sorted(glob(os.path.join(log_path, "*.json")), key=sort_key)

    use_cache = False

    Sumz([f for f in files if "zipf1" in f], "Zipf1", use_cache=use_cache)
    Sumz([f for f in files if "cloud" in f], "CloudPhysics", use_cache=use_cache)
    Sumz([f for f in files if "metacdn" in f], "MetaCDN", use_cache=use_cache)


if __name__ == "__main__":
    main()
