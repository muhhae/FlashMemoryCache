import docs_writer as w
import pandas as pd
import plotly.express as px
import plotly.io as pio
import plotly.graph_objs as go

pio.templates.default = "plotly_dark"

out_path = "../../"
docs = out_path + "docs/"
result = out_path + "result/"

cache_sizes = [0.01, 0.1, 0.2, 0.4]
for s in cache_sizes:
    md = open(result + f"datasets[{s}].md", "w")
    html = open(docs + f"datasets[{s}].html", "w")
    w.Write(md, html, f"# DATASETS FEATURES {s} \n")
    datasets = f"../../build/result/datasets/small_zipf[{s}].csv"
    df = pd.read_csv(datasets)
    df = df.sample(frac=0.1, random_state=1)
    df["wasted"] = df["wasted"].astype("category")
    cols = df.columns
    for c in cols:
        if c in ["wasted", "obj_id"]:
            continue
        w.Write(md, html, f"## {c}  \n")
        fig = px.scatter(
            df,
            x=c,
            y="obj_id",
            color="wasted",
        )
        fig.update_layout(
            xaxis_title=c,
            yaxis_title="obj_id",
            font=dict(size=14),
            height=800,
            width=1000,
        )
        fig.update_xaxes(showgrid=True)
        fig.update_yaxes(showgrid=True)
        w.WriteFig(md, html, fig)
        w.Write(md, html, "  \n")
    w.WriteHTML(html)
