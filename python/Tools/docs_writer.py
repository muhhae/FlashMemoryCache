from _plotly_utils.utils import base64
import markdown as MD

from plotly.graph_objs import Figure


class DocsWriter:
    def __init__(
        self, html_path: str | None = None, md_path: str | None = None
    ) -> None:
        self.html_file = open(html_path, "w") if html_path is not None else None
        self.md_file = open(md_path, "w") if md_path is not None else None

        self.html_content = ""
        self.md_content = ""
        self.counter = 0

    def Write(self, content: str):
        self.html_content += content * (self.html_file is not None)
        self.md_content += content * (self.html_file is not None)

    def WriteFig(self, fig: Figure):
        if self.md_file is not None:
            png_bytes = fig.to_image(format="png")
            b64 = base64.b64encode(png_bytes)
            uri = f"data:image/png;base64,{b64.decode('utf-8')}"
            self.md_content += f'<img src="{uri}" alt="Plotly Chart">'
        if self.html_file is not None:
            j = fig.to_json()
            self.html_content += f"""
<div class="chart-wrapper">
    <div id="id-{self.counter}" class="chart-placeholder" data-chart-json='{j}'>
        Chart placeholder
    </div>
</div>
"""
        self.counter += 1

    def Flush(self):
        if self.md_file is not None:
            self.md_file.write(self.md_content)
            self.md_file.close()
        if self.html_file is not None:
            md = MD.Markdown(
                extensions=["extra", "toc"],
            )
            html_body = md.convert(self.html_content)
            self.html_file.write(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <script src='https://cdn.plot.ly/plotly-3.0.1.min.js' charset='utf-8'></script>
    <script src="https://d3js.org/d3.v7.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/mathjax/3.2.2/es5/core.min.js" integrity="sha512-Vj8DsxZwse5LgmhPlIXhSr/+mwl8OajbZVCr4mX/TcDjwU1ijG6A15cnyRXqZd2mUOQqRk4YbQdc7XhvedWqMg==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
    <script src='../scripts/main.js' charset='utf-8'></script>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/github-markdown-css/5.8.1/github-markdown.min.css" integrity="sha512-BrOPA520KmDMqieeM7XFe6a3u3Sb3F1JBaQnrIAmWg3EYrciJ+Qqe6ZcKCdfPv26rGcgTrJnZ/IdQEct8h3Zhw==" crossorigin="anonymous" referrerpolicy="no-referrer" />
    <link rel="stylesheet" href="../styles/main.css"/>
</head>
<body>
<button class="sidebar-toggle-btn" id="sidebarToggleBtn" aria-label="Toggle sidebar">&#9776;</button>
<nav class="sidenav" id="sidebar">
    <h2>Table of Contents</h2>
    {md.toc}
</nav>
<main class="content">
<article class="markdown-body">
{html_body}
</article>
</main>
</body>
</html>
            """)
            self.html_file.close()
