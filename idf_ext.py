import copy
import os

import serial
import serial.tools.list_ports

import idf_py_actions.serial_ext as serial_ext

THIS_SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))


def action_extensions(base_actions, project_path):
    ext_actions = serial_ext.action_extensions(base_actions, project_path)

    original_run_tool = serial_ext.run_tool

    calling_monitori = False
    calling_monitorf = False

    def run_tool(tool_name, args, cwd, env=dict(), custom_error_handler=None):
        if tool_name == "idf_monitor":
            assert os.path.basename(args[1]) == "idf_monitor.py"
            if calling_monitori:
                idf_monitori = os.path.join(THIS_SCRIPT_DIR, "tools/idf_monitori.py")
                args[1] = idf_monitori
                original_run_tool("idf_monitori", args, cwd, env, custom_error_handler)
            elif calling_monitorf:
                idf_monitorf = os.path.join(THIS_SCRIPT_DIR, "tools/idf_monitorf.py")
                args[1] = idf_monitorf
                original_run_tool("idf_monitorf", args, cwd, env, custom_error_handler)
        else:
            original_run_tool(tool_name, args, cwd, env, custom_error_handler)

    #
    # Override run_tool with our own run_tool, which intercepts calls to our
    # idf.py monitor variant actions and runs the corresponding action variant
    # script
    #
    serial_ext.run_tool = run_tool

    actions = {
        "actions": {
            "monitori": copy.deepcopy(ext_actions["actions"]["monitor"]),
            "monitorf": copy.deepcopy(ext_actions["actions"]["monitor"]),
        }
    }
    actions["actions"]["monitori"][
        "help"
    ] = "Same as monitor, but supports interactive Python script with Ctrl+E"
    actions["actions"]["monitorf"][
        "help"
    ] = "Same as monitor, but redirects output also to another pty"
    original_callback = actions["actions"]["monitori"]["callback"]

    def monitori_callback(*args, **kwargs):
        nonlocal calling_monitori
        calling_monitori = True
        original_callback(*args, **kwargs)
        calling_monitori = False

    def monitorf_callback(*args, **kwargs):
        nonlocal calling_monitorf
        calling_monitorf = True
        original_callback(*args, **kwargs)
        calling_monitorf = False

    actions["actions"]["monitori"]["callback"] = monitori_callback
    actions["actions"]["monitorf"]["callback"] = monitorf_callback
    return actions
