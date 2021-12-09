import httpimport

with httpimport.remote_repo(["cellworld_py_setup"], "https://raw.githubusercontent.com/germanespinosa/cellworld_py/master/"):
    import cellworld_py_setup
cellworld_py_setup.install(version="1.3", force=True)

with httpimport.remote_repo(["agent_tracking_py_setup"], "https://raw.githubusercontent.com/germanespinosa/agent_tracking/master/python/"):
    import agent_tracking_py_setup
agent_tracking_py_setup.install(version="1.1.002", force=True)
