import copy
import os

import serial
import serial.tools.list_ports

import idf_py_actions.serial_ext as serial_ext

THIS_SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))


def action_extensions(base_actions, project_path):
    ext_actions = serial_ext.action_extensions(base_actions, project_path)

    original_run_tool = serial_ext.run_tool

    monitori = {
        "calling": False,
        "args": [],
        "kwargs": {},
    }
    monitorf = {
        "calling": False,
        "args": [],
        "kwargs": {},
    }

    def run_tool(tool_name, args, cwd, env=dict(), custom_error_handler=None):
        if tool_name == "idf_monitor":
            assert os.path.basename(args[1]) == "idf_monitor.py"
            if monitori["calling"] == True:
                idf_monitori = os.path.join(THIS_SCRIPT_DIR, "tools/idf_monitori.py")
                args[1] = idf_monitori
                if monitori["kwargs"].get("hide_erpc", False):
                    args.append("--hide-erpc")
                original_run_tool("idf_monitori", args, cwd, env, custom_error_handler)
            elif monitorf["calling"] == True:
                idf_monitorf = os.path.join(THIS_SCRIPT_DIR, "tools/idf_monitorf.py")
                args[1] = idf_monitorf
                if monitorf["kwargs"].get("hide_erpc", False):
                    args.append("--hide-erpc")
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
        nonlocal monitori
        monitori["calling"] = True
        monitori["args"] = args
        monitori["kwargs"] = copy.deepcopy(kwargs)

        kwargs.pop("hide_erpc", None)
        original_callback(*args, **kwargs)
        monitori["calling"] = False

    def monitorf_callback(*args, **kwargs):
        nonlocal monitorf
        monitorf["calling"] = True
        monitorf["args"] = args
        monitorf["kwargs"] = copy.deepcopy(kwargs)

        kwargs.pop("hide_erpc", None)
        original_callback(*args, **kwargs)

        monitorf["calling"] = False

    hide_erpc_option = {
        "names": ["--hide-erpc"],
        "is_flag": True,
        "default": False,
        "help": ("Whether to hide logs that contain eRPC payload"),
    }

    actions["actions"]["monitori"]["callback"] = monitori_callback
    actions["actions"]["monitori"]["options"].append(hide_erpc_option)
    actions["actions"]["monitorf"]["callback"] = monitorf_callback
    actions["actions"]["monitorf"]["options"].append(hide_erpc_option)

    return actions
