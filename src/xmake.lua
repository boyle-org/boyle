-- $(projectdir) resolves to a synthetic wrapper project root when built via
-- the xmake-python PEP 517 backend (pyproject.xmake.toml); os.scriptdir()
-- stays correct regardless, so derive the repo root from it directly.
add_includedirs(path.join(path.directory(os.scriptdir()), "src"), {public = true})

includes("boyle/xmake.lua")
