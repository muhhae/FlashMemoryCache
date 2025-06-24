import markdown as MD
import typing as T


def printls(ls: T.List):
    for x in ls:
        print(x)


g_html_content = ""


def Write(md: T.TextIO, html: T.TextIO, content: str):
    global g_html_content
    md.write(content)
    g_html_content += content


def WriteFig(md: T.TextIO, html: T.TextIO, fig):
    global g_html_content
    md.write(fig.to_image(format="svg").decode("utf-8") + "  \n\n")
    g_html_content += fig.to_html(full_html=False, include_plotlyjs=False) + "  \n\n"


def WriteHTML(html: T.TextIO):
    global g_html_content
    md = MD.Markdown(
        extensions=["extra", "toc"],
    )
    html_body = md.convert(g_html_content)
    toc = md.toc
    html.write(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <script src='https://cdn.plot.ly/plotly-3.0.1.min.js' charset='utf-8'></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/mathjax/3.2.2/es5/core.min.js" integrity="sha512-Vj8DsxZwse5LgmhPlIXhSr/+mwl8OajbZVCr4mX/TcDjwU1ijG6A15cnyRXqZd2mUOQqRk4YbQdc7XhvedWqMg==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/github-markdown-css/5.8.1/github-markdown.min.css" integrity="sha512-BrOPA520KmDMqieeM7XFe6a3u3Sb3F1JBaQnrIAmWg3EYrciJ+Qqe6ZcKCdfPv26rGcgTrJnZ/IdQEct8h3Zhw==" crossorigin="anonymous" referrerpolicy="no-referrer" />
    <style>
    :root {{
        /* Light mode defaults */
        --bg-color: #fff;
        --sidebar-bg: #f6f8fa;
        --text-color: #222;
        --sidebar-link: #444;
        --sidebar-link-hover: #111;
        --sidebar-heading: #333;
        --sidebar-marker: #444;
    }}
    @media (prefers-color-scheme: dark) {{
        :root {{
            --bg-color: #111;
            --sidebar-bg: #111;
            --text-color: #eee;
            --sidebar-link: #818181;
            --sidebar-link-hover: #f1f1f1;
            --sidebar-heading: #818181;
            --sidebar-marker: #818181;
        }}
    }}
    body {{
        background-color: var(--bg-color);
        color: var(--text-color);
        display: flex;
        margin: 0;
    }}
    .sidenav {{
        height: 100%;
        width: 280px;
        min-width: 200px;
        max-width: 300px;
        position: fixed;
        z-index: 1;
        top: 0;
        left: 0;
        background-color: var(--sidebar-bg);
        overflow-x: hidden;
        overflow-y: auto;
        padding: 2em 1em 2em 2em;
        transition: transform 0.3s ease;
    }}
    .sidenav.closed {{
        transform: translateX(-100%);
    }}
    .sidenav a {{
        padding: 1px 1px 1px 1px;
        font-size: 16px;
        color: var(--sidebar-link);
        display: block;
        text-decoration: none;
        transition: color 0.2s;
    }}
    .sidenav h2 {{
        padding: 1px 1px 1px 1px;
        font-size: 20px;
        color: var(--sidebar-heading);
        display: block;
    }}
    .sidenav li::marker {{
        color: var(--sidebar-marker);
    }}
    .sidenav a:hover {{
        color: var(--sidebar-link-hover);
    }}
    .sidebar-toggle-btn {{
        position: fixed;
        top: 10px;
        left: 10px;
        z-index: 1002;
        background: var(--sidebar-bg);
        color: var(--sidebar-link);
        border: 1px solid var(--sidebar-link);
        border-radius: 4px;
        padding: 8px 14px;
        cursor: pointer;
        font-size: 18px;
        transition: background 0.2s, color 0.2s, left 0.3s ease;
    }}
    .sidebar-toggle-btn:hover {{
        background: var(--sidebar-link-hover);
        color: var(--bg-color);
    }}
    .sidenav.closed ~ .content {{
        margin-left: 0;
    }}
    .sidenav.closed ~ .sidebar-toggle-btn {{
        left: 10px;
    }}
    .content {{
        margin-left: 300px;
        padding: 2em;
        width: 100%;
        transition: margin-left 0.3s ease;
    }}
    .markdown-body {{
        box-sizing: border-box;
        min-width: 200px;
        max-width: 980px;
        margin: 0 auto;
        padding: 45px;
    }}
    @media (max-width: 767px) {{
        .markdown-body {{
            padding: 15px;
        }}
    }}

    /* --- NEW CSS FOR COLLAPSIBLE TOC ITEMS --- */
    .sidenav ul {{
        list-style-type: none;
        padding-left: 1em;
    }}
    .sidenav li > ul {{
        display: none; /* Hide sub-menus by default */
        padding-left: 20px;
    }}
    .sidenav li.open > ul {{
        display: block; /* Show sub-menus when parent is open */
    }}
    .sidenav li > a.collapsible {{
        cursor: pointer;
        position: relative;
    }}
    /* Arrow indicator */
    .sidenav li > a.collapsible::before {{
        content: '►';
        position: absolute;
        left: -1em;
        font-size: 0.7em;
        top: 0.4em;
        transition: transform 0.2s ease-in-out;
        color: var(--sidebar-marker);
    }}
    .sidenav li.open > a.collapsible::before {{
        transform: rotate(90deg);
    }}

</style>
</head>
<body>
<button class="sidebar-toggle-btn" id="sidebarToggleBtn" aria-label="Toggle sidebar">&#9776;</button>
<nav class="sidenav" id="sidebar">
    <h2>Table of Contents</h2>
    {toc}
</nav>
<script>
    const sidebar = document.getElementById('sidebar');
    const btn = document.getElementById('sidebarToggleBtn');
    const content = document.querySelector('.content');

    // --- Sidebar toggle script ---
    btn.addEventListener('click', function(){{
        sidebar.classList.toggle('closed');
        if (sidebar.classList.contains('closed')) {{
            btn.setAttribute('aria-label', 'Open sidebar');
        }} else {{
            btn.setAttribute('aria-label', 'Close sidebar');
        }}
    }});

    if(window.innerWidth <= 700){{
        sidebar.classList.add('closed');
    }}

    // --- NEW JAVASCRIPT FOR COLLAPSIBLE TOC ITEMS ---
    document.querySelectorAll('.sidenav li').forEach(item => {{
        // Check if this list item has a sub-list (a ul element)
        const sublist = item.querySelector('ul');
        if (sublist) {{
            // Find the first anchor tag to be the click trigger
            const link = item.querySelector('a');
            if(link) {{
                link.classList.add('collapsible'); // Add class for styling

                link.addEventListener('click', (e) => {{
                    // Prevent default link behavior only if it's a placeholder link
                    if (link.getAttribute('href') === '#') {{
                       e.preventDefault();
                    }}
                    // Toggle the 'open' class on the parent LI
                    item.classList.toggle('open');
                }});
            }}
        }}
    }});

</script>
<main class="content">
<article class="markdown-body">
{html_body}
</article>
</main>
</body>
</html>
""")
    g_html_content = ""
