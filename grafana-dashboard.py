#!/usr/bin/env python3
"""Generate Grafana dashboard for PtokaX DC++ Hub metrics."""
import json
import os
import sys
import urllib.request

GRAFANA_URL = "http://localhost:3000"
GRAFANA_USER = "admin"
GRAFANA_PASS = os.environ.get("GRAFANA_ADMIN_PASSWORD", "1")

def make_panel(title, targets, grid_pos, chart_type="timeseries", unit="none"):
    return {
        "type": chart_type,
        "title": title,
        "gridPos": grid_pos,
        "targets": [{"expr": t, "legendFormat": l, "refId": str(i)} for i, (t, l) in enumerate(targets)],
        "fieldConfig": {
            "defaults": {
                "color": {"mode": "palette-classic"},
                "custom": {
                    "drawStyle": "line",
                    "lineInterpolation": "smooth",
                    "fillOpacity": 10,
                    "lineWidth": 1,
                },
                "unit": unit,
                "decimals": 1,
            },
            "overrides": [],
        },
        "options": {
            "legend": {"displayMode": "list", "placement": "bottom"},
            "tooltip": {"mode": "multi"},
        },
    }

def make_stat(title, targets, grid_pos, unit="none"):
    return make_panel(title, targets, grid_pos, "stat", unit)

def build_dashboard():
    panels = [
        # Row 0: Core user stats
        make_stat("Users Online",
            [("flylinkdc_hub_users_online", "")],
            {"h": 4, "w": 4, "x": 0, "y": 0}),
        make_stat("Users Logged In",
            [("flylinkdc_hub_users_logged_in", "")],
            {"h": 4, "w": 4, "x": 4, "y": 0}),
        make_stat("Users Connecting",
            [("flylinkdc_hub_users_connecting", "")],
            {"h": 4, "w": 4, "x": 8, "y": 0}),
        make_stat("Operators Online",
            [("flylinkdc_hub_operators_online", "")],
            {"h": 4, "w": 4, "x": 12, "y": 0}),
        make_stat("Peak Users",
            [("flylinkdc_hub_users_peak", "")],
            {"h": 4, "w": 4, "x": 16, "y": 0}),
        make_stat("Registered Users",
            [("flylinkdc_hub_users_registered", "")],
            {"h": 4, "w": 4, "x": 20, "y": 0}),

        # Row 1: User composition
        make_stat("Users Hidden",
            [("flylinkdc_hub_users_hidden", "")],
            {"h": 4, "w": 4, "x": 0, "y": 4}),
        make_stat("Users Gagged",
            [("flylinkdc_hub_users_gagged", "")],
            {"h": 4, "w": 4, "x": 4, "y": 4}),
        make_stat("Users Active",
            [("flylinkdc_hub_users_active", "")],
            {"h": 4, "w": 4, "x": 8, "y": 4}),
        make_stat("IPv6 Users",
            [("flylinkdc_hub_users_ipv6", "")],
            {"h": 4, "w": 4, "x": 12, "y": 4}),
        make_stat("Users Sharing",
            [("flylinkdc_hub_users_sharing", "")],
            {"h": 4, "w": 4, "x": 16, "y": 4}),
        make_stat("Bans",
            [("flylinkdc_hub_bans_count", "{{type}}")],
            {"h": 4, "w": 4, "x": 20, "y": 4}),

        # Row 2: Resource stats
        make_stat("Config DB Size",
            [("flylinkdc_hub_sqlite_size_bytes", "")],
            {"h": 4, "w": 8, "x": 0, "y": 8}, "bytes"),
        make_stat("Share Size",
            [("flylinkdc_hub_share_bytes", "")],
            {"h": 4, "w": 8, "x": 8, "y": 8}, "bytes"),
        make_stat("Memory (RSS)",
            [("flylinkdc_hub_rusage{resource=\"ru_maxrss\"}", "")],
            {"h": 4, "w": 8, "x": 16, "y": 8}, "kbytes"),

        # Row 3: Hub uptime
        make_stat("Hub Uptime",
            [("(time() - flylinkdc_hub_start_time_seconds)", "")],
            {"h": 4, "w": 24, "x": 0, "y": 12}, "dtdurations"),

        # Row 4: Server info
        make_stat("Scripts / Bots",
            [("flylinkdc_hub_scripts_count", "scripts"),
             ("flylinkdc_hub_bots_count", "bots")],
            {"h": 4, "w": 6, "x": 0, "y": 16}),
        make_stat("Profiles",
            [("flylinkdc_hub_profiles_count", "")],
            {"h": 4, "w": 6, "x": 6, "y": 16}),
        make_stat("Compression Saved",
            [("flylinkdc_hub_compression_saved_bytes", "")],
            {"h": 4, "w": 6, "x": 12, "y": 16}, "bytes"),
        make_stat("Total Slots",
            [("flylinkdc_hub_total_slots", "")],
            {"h": 4, "w": 6, "x": 18, "y": 16}),

        # Row 5: Security & errors
        make_panel("Protocol Errors",
            [("flylinkdc_hub_protocol_errors_total", "{{error}}")],
            {"h": 6, "w": 12, "x": 0, "y": 20}),
        make_panel("Security Events",
            [("flylinkdc_hub_connection_flood_total", "connection_flood"),
             ("flylinkdc_hub_bruteforce_attempts_total", "bruteforce"),
             ("flylinkdc_hub_fast_reconnect_total", "fast_reconnect")],
            {"h": 6, "w": 12, "x": 12, "y": 20}),

        # Row 7: Connection rates
        make_panel("Joins / Parts Rate",
            [("rate(flylinkdc_hub_joins_total[1m])", "joins"),
             ("rate(flylinkdc_hub_parts_total[1m])", "parts")],
            {"h": 6, "w": 12, "x": 0, "y": 26}),
        make_panel("DC++ Commands Rate",
            [("rate(flylinkdc_hub_dc_commands_total[1m])", "{{command}}")],
            {"h": 6, "w": 12, "x": 12, "y": 26}),

        # Row 8: Protocol commands breakdown
        make_panel("DC Protocol Commands",
            [("flylinkdc_hub_command_stats", "{{command}}")],
            {"h": 6, "w": 12, "x": 0, "y": 32}),
        make_panel("Searches",
            [("flylinkdc_hub_active_searches", "active"),
             ("flylinkdc_hub_passive_searches", "passive")],
            {"h": 6, "w": 12, "x": 12, "y": 32}),

        # Row 9: Lua
        make_panel("Lua Calls Rate",
            [("rate(flylinkdc_hub_lua_calls_total[1m])", "{{func}}")],
            {"h": 6, "w": 8, "x": 0, "y": 38}),
        make_panel("Lua Memory per Script",
            [("flylinkdc_hub_lua_memory_bytes", "{{script}}")],
            {"h": 6, "w": 8, "x": 8, "y": 38}, unit="bytes"),
        make_panel("Lua Calls per Script",
            [("rate(flylinkdc_hub_lua_call_count[1m])", "{{script}}")],
            {"h": 6, "w": 8, "x": 16, "y": 38}),

        # Row 10: Bandwidth
        make_panel("Bandwidth",
            [("flylinkdc_hub_bandwidth_bytes_per_sec/1048576", "{{direction}}")],
            {"h": 6, "w": 12, "x": 0, "y": 44}, unit="short"),
        make_panel("Event Queue",
            [("flylinkdc_hub_event_queue_depth", "depth")],
            {"h": 6, "w": 12, "x": 12, "y": 44}),

        # Row 11: Traffic
        make_panel("Bytes Received Rate",
            [("rate(flylinkdc_hub_recv_bytes_total[1m])/1048576", "{{type}}")],
            {"h": 6, "w": 12, "x": 0, "y": 50}, unit="short"),
        make_panel("Bytes Sent Rate",
            [("rate(flylinkdc_hub_send_bytes_total[1m])/1048576", "{{type}}")],
            {"h": 6, "w": 12, "x": 12, "y": 50}, unit="short"),

        # Row 12: Network
        make_panel("Packets Rate",
            [("rate(flylinkdc_hub_recv_packets_total[1m])", "recv {{type}}"),
             ("rate(flylinkdc_hub_send_packets_total[1m])", "send {{type}}")],
            {"h": 6, "w": 12, "x": 0, "y": 56}),
        make_panel("Compression",
            [("rate(flylinkdc_hub_compress_bytes_total[1m])/1048576", "{{type}}")],
            {"h": 6, "w": 12, "x": 12, "y": 56}, unit="short"),

        # Row 13: Users by profile + Queue
        make_panel("Users by Profile",
            [("flylinkdc_hub_users_by_profile", "{{profile}}")],
            {"h": 6, "w": 12, "x": 0, "y": 62}),
        make_panel("IP Spread / Queue",
            [("flylinkdc_hub_ip_spread", "max_conn_per_ip"),
             ("flylinkdc_hub_event_queue_depth", "event_queue")],
            {"h": 6, "w": 12, "x": 12, "y": 62}),

        # Row 14: Internal state
        make_stat("CPU Usage",
            [("flylinkdc_hub_cpu_usage_percent", "")],
            {"h": 4, "w": 4, "x": 0, "y": 68}, "percent"),
        make_stat("Lua Timers",
            [("flylinkdc_hub_lua_timers_active", "")],
            {"h": 4, "w": 4, "x": 4, "y": 68}),
        make_stat("Reserved Nicks",
            [("flylinkdc_hub_reserved_nicks", "")],
            {"h": 4, "w": 4, "x": 8, "y": 68}),
        make_stat("UDP Debug Subs",
            [("flylinkdc_hub_udp_debug_subscribers", "")],
            {"h": 4, "w": 4, "x": 12, "y": 68}),
        make_stat("IP2Country IPv4 / IPv6",
            [("flylinkdc_hub_ip2country_ranges{family=\"ipv4\"}", "ipv4"),
             ("flylinkdc_hub_ip2country_ranges{family=\"ipv6\"}", "ipv6")],
            {"h": 4, "w": 4, "x": 16, "y": 68}),
        make_stat("Con Flood Watchlist",
            [("flylinkdc_hub_con_flood_watchlist", "")],
            {"h": 4, "w": 4, "x": 20, "y": 68}),

        # Row 15: Queue buffers
        make_panel("Global Queue Buffer",
            [("flylinkdc_hub_global_queue_buffer_bytes", "{{queue}}")],
            {"h": 6, "w": 12, "x": 0, "y": 72}, unit="bytes"),
        make_panel("Users List Buffer",
            [("flylinkdc_hub_users_list_buffer_bytes", "{{list}}")],
            {"h": 6, "w": 12, "x": 12, "y": 72}, unit="bytes"),

        # Row 16: More buffers + rests
        make_panel("User Send/Recv Buffer",
            [("flylinkdc_hub_users_buffer_bytes", "{{buffer}}")],
            {"h": 6, "w": 12, "x": 0, "y": 78}, unit="bytes"),
        make_panel("Send/Recv Rests",
            [("flylinkdc_hub_send_rests", "send"),
             ("flylinkdc_hub_send_rests_peak", "send_peak"),
             ("flylinkdc_hub_recv_rests", "recv"),
             ("flylinkdc_hub_recv_rests_peak", "recv_peak")],
            {"h": 6, "w": 12, "x": 12, "y": 78}),

        # Row 17: State detail
        make_panel("Users by State",
            [("flylinkdc_hub_users_by_state", "{{state}}")],
            {"h": 6, "w": 12, "x": 0, "y": 84}),
        make_panel("DC Protocol Commands (all)",
            [("flylinkdc_hub_command_stats{command=~\"unknown|opforcemove|mypass|getinfo|getnicklist|kick|botinfo|zpipe|multisearch|multiconnecttome|close|extjson\"}", "{{command}}")],
            {"h": 6, "w": 12, "x": 12, "y": 84}),

        # Row 18
        make_panel("System Resources Detail",
            [("flylinkdc_hub_rusage{resource=~\"ru_minflt|ru_majflt|ru_nvcsw|ru_nivcsw\"}", "{{resource}}")],
            {"h": 6, "w": 12, "x": 0, "y": 90}),
        make_panel("Lua User Values",
            [("rate(flylinkdc_hub_lua_user_value_total[1m])", "{{value}}")],
            {"h": 6, "w": 12, "x": 12, "y": 90}),

        # Row 19
        make_panel("Lua User Data",
            [("rate(flylinkdc_hub_lua_userdata_total[1m])", "{{value}}")],
            {"h": 6, "w": 24, "x": 0, "y": 96}),

        # Row 20: Lua time per script
        make_panel("Lua Time per Script",
            [("rate(flylinkdc_hub_lua_time_nsec[1m])", "{{script}}")],
            {"h": 6, "w": 24, "x": 0, "y": 102}, unit="ns"),

        # Row 21: Log analytics
        make_panel("Log Rate",
            [('_stream:{service="ptokax"} | count() per 1m', "")],
            {"h": 6, "w": 8, "x": 0, "y": 108}),
        make_panel("Error Log Rate",
            [('_stream:{service="ptokax"} _msg:"[ERR]" | count() per 1m', "")],
            {"h": 6, "w": 8, "x": 8, "y": 108}),
        make_panel("Log Volume by File",
            [('_stream:{service="ptokax"} | count() per 1m by (file)', "{{file}}")],
            {"h": 6, "w": 8, "x": 16, "y": 108}),

        # Row 22: Logs
        {
            "type": "logs",
            "title": "Hub Logs",
            "gridPos": {"h": 10, "w": 24, "x": 0, "y": 114},
            "targets": [{
                "expr": '_stream:{service="ptokax"}',
                "refId": "A",
                "datasource": {"type": "victoriametrics-logs-datasource", "uid": "victorialogs"},
            }],
            "options": {
                "legend": {"displayMode": "list", "placement": "bottom"},
                "tooltip": {"mode": "multi"},
            },
        },

        # Row 23: Command latency
        make_panel("Command Latency",
            [("rate(flylinkdc_hub_command_latency_nsec{command!~\"other|processcmds\"}[1m])", "{{command}}")],
            {"h": 6, "w": 12, "x": 0, "y": 124}, unit="ns"),
        make_panel("Command Latency (other)",
            [("rate(flylinkdc_hub_command_latency_nsec{command=~\"other|processcmds\"}[1m])", "{{command}}")],
            {"h": 6, "w": 12, "x": 12, "y": 124}, unit="ns"),

        # Row 24: Connections + fd + pending + config
        make_stat("Connections Accepted",
            [("rate(flylinkdc_hub_connections_accepted[1m])", "")],
            {"h": 4, "w": 6, "x": 0, "y": 130}),
        make_stat("Connections Closed",
            [("rate(flylinkdc_hub_connections_closed[1m])", "")],
            {"h": 4, "w": 6, "x": 6, "y": 130}),
        make_stat("FD Count",
            [("flylinkdc_hub_fd_count", "")],
            {"h": 4, "w": 6, "x": 12, "y": 130}),
        make_stat("Pending Connections",
            [("flylinkdc_hub_pending_connections", "")],
            {"h": 4, "w": 6, "x": 18, "y": 130}),

        # Row 25: Lua GC + timers per script
        make_panel("Lua GC Pause per Script",
            [("flylinkdc_hub_lua_gc_pause_nsec", "{{script}}")],
            {"h": 6, "w": 12, "x": 0, "y": 134}, unit="ns"),
        make_panel("Lua Timers per Script",
            [("flylinkdc_hub_lua_timers_per_script", "{{script}}")],
            {"h": 6, "w": 12, "x": 12, "y": 134}),
        make_panel("Test Port Queries",
            [("flylinkdc_hub_test_port_queries_total", "queries")],
            {"h": 6, "w": 12, "x": 0, "y": 140}),
        make_panel("Test Port Bytes I/O",
            [("flylinkdc_hub_test_port_bytes_in_total", "in"),
             ("flylinkdc_hub_test_port_bytes_out_total", "out")],
            {"h": 6, "w": 12, "x": 12, "y": 140}),
        make_panel("Test Port Compressed Bytes",
            [("flylinkdc_hub_test_port_compressed_bytes_in_total", "in"),
             ("flylinkdc_hub_test_port_compressed_bytes_out_total", "out")],
            {"h": 6, "w": 12, "x": 0, "y": 146}),
    ]

    return {
        "title": "PtokaX DC++ Hub",
        "uid": "ptokax-hub-v2",
        "tags": ["ptokax", "dchub"],
        "timezone": "browser",
        "panels": panels,
        "time": {"from": "now-1h", "to": "now"},
        "refresh": "5s",
        "schemaVersion": 39,
        "editable": True,
    }

def main():
    url = sys.argv[1] if len(sys.argv) > 1 else GRAFANA_URL
    user = sys.argv[2] if len(sys.argv) > 2 else GRAFANA_USER
    passwd = sys.argv[3] if len(sys.argv) > 3 else GRAFANA_PASS

    dashboard = build_dashboard()

    import base64
    auth = base64.b64encode(f"{user}:{passwd}".encode()).decode()

    # Save provisioning file
    prov_path = "/home/dc/flylinkdc-hub/grafana/provisioning/dashboards/hub-overview.json"
    with open(prov_path, "w") as f:
        json.dump(dashboard, f, indent=2)
    print(f"Provisioning file saved: {prov_path}")

    # Upload to Grafana API
    payload = {"dashboard": dashboard, "overwrite": True}
    data = json.dumps(payload).encode()
    req = urllib.request.Request(
        f"{url}/api/dashboards/db",
        data=data,
        headers={
            "Content-Type": "application/json",
            "Authorization": f"Basic {auth}",
        },
    )

    try:
        resp = urllib.request.urlopen(req)
        result = json.loads(resp.read())
        print(f"Dashboard created: {url}/d/{result.get('uid', 'ptokax-hub')}")
    except urllib.error.HTTPError as e:
        body = e.read().decode()
        print(f"Error {e.code}: {body}")
        sys.exit(1)

if __name__ == "__main__":
    main()
