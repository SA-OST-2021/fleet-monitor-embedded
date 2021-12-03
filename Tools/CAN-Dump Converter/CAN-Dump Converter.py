# -*- coding: utf-8 -*-
"""
Created on Fri Dec  3 10:52:56 2021

@author: florian.baumgartner
"""

import numpy as np
import pandas as pd
import plotly.io as pio
import plotly.graph_objects as go
from plotly.subplots import make_subplots

pio.renderers.default = "browser"
pd.options.mode.chained_assignment = None

fileName = "Dump_211117.txt"

pgnNameTable = {
    'FEE9': "Fuel Consumption: LFC",
    'FEFC': "Dash Display 1: DD1",
    'F004': "Electronic Engine Controller #1: EEC1",
    'FEE5': "Engine Hours, Revolutions: HOURS",
    'FEEC': "Vehicle Identification: VI",
    'FDD1': "MS-standard Interface Identity / Capabilities: FMS",
    'FEC1': "High Resolution Vehicle Distance: VDHR",
    'FE6C': "Tachograph : TCO1",
    'FEEE': "Engine Temperature 1: ET1",
    'FEF5': "Ambient Conditions: AMB",
    'FE6B': "Driver's Identification: DI",
    'FEF2': "Fuel Economy: LFE",
    'FEAE': "Air Supply Pressure : AIR1",
    'FD09': "High Resolution Fuel Consumption (Liquid): HRLFC",
    'FE56': "Aftertreatment 1 Diesel Exhaust Fluid Tank 1 Information: AT1T1I",
    'FD7D': "FMS Tell Tale Status: FMS1",
    'F001': "Electronic Brake Controller 1: EBC1",
    'FDC2': "Electronic Engine Controller 14: EEC14",
    'FEAF': "Fuel Consumption (Gaseous): GFC",
    'F000': "Electronic Retarder Controller 1: ERC1",
    'FEF1': "Cruise Control/Vehicle Speed 1: CCVS1",
    'F003': "Electronic Engine Controller #2: EEC2",
    'FEEA': "Vehicle Weight: VW",
    'FEC0': "Service Information: SERV",
    'FDA4': "PTO Drive Engagement: PTODE",
    'FE70': "Combination Vehicle Weight: CVW",
    'FE4E': "Door Control 1: DC1",
    'FDA5': "Door Control 2: DC2",
    'FEE6': "Time / Date : TD",
    'FED5': "Alternator Speed : AS",
    'F005': "Electronic Transmission Controller 2 : ETC2",
    'FE58': "Air Suspension Control 4 : ASC4",
    'FCB7': "Vehicle Electrical Power #4 : VEP4",
    'F009': "Vehicle Dynamic Stability Control 2 : VDC2",
}


data = []
with open(fileName) as file:
    for l in file.readlines():
        l = l.split()
        s = {"time":     float(l[0].strip("()")),
             "priority": int(l[2][:2], 16),
             "pgn":      int(l[2][2:6], 16),
             "source":   int(l[2][6:], 16),
             "data":     bytearray([int(d, 16) for d in l[4:]])}
        for n in range(8):
            s[f"{n}"] = int(l[4 + n], 16)
        data.append(s)


df = pd.DataFrame(data, columns=["time", "priority", "pgn", "source", "data",
                                 "0", "1", "2", "3", "4", "5", "6", "7"])
df["date"] = pd.to_datetime(df["time"], unit="s")
pgnTypes = list(np.unique(df[["pgn"]].values))
for pgn in pgnTypes:
    if(f"{pgn:04X}" in pgnNameTable):
        print(f"{pgn:04X}: " + pgnNameTable[f"{pgn:04X}"])
    else:
        print(f"{pgn:04X}")


tachograph = df[df["pgn"] == 0xFE6C]
tachograph["speed"] = tachograph["7"] + tachograph["6"] / 256.0

fuelConsumption = df[df["pgn"] == 0xFD09]
fuelConsumption["fuelConsumption"] = (fuelConsumption["4"] + 
                                      fuelConsumption["5"] * 256 + 
                                      fuelConsumption["6"] * 256**2  +
                                      fuelConsumption["7"] * 256**3) / 1000.0
fuelConsumption["fuelConsumption"] -= fuelConsumption["fuelConsumption"].iloc[0]

doors = df[df["pgn"] == 0xFDA5]
doorStatus = {0x00: "closed", 0x01: "open", 0x02: "error", 0x03: "not available"}
doors["door1"] = doors.replace({"0": doorStatus})

# (doors["0"] & 0x0C).values >> 2)



fig = make_subplots(rows=4, cols=1)

fig.add_trace(go.Scatter(x=tachograph["date"], y=tachograph["speed"],
                         name="Vehicle Speed [km/h]",
                         fill="tozeroy"), row=1, col=1)

fig.add_trace(go.Scatter(x=tachograph["date"], y=fuelConsumption["fuelConsumption"],
                         name="Fuel Consumption [l]",
                         fill="tozeroy"), row=2, col=1)

# fig.add_trace(go.Scatter(x=tachograph["date"], y=tachograph["speed"],
#                          name="Vehicle Speed [km/h]",
#                          fill="tozeroy"), row=3, col=1)

# fig.add_trace(go.Scatter(x=tachograph["date"], y=tachograph["speed"],
#                          name="Vehicle Speed [km/h]",
#                          fill="tozeroy"), row=4, col=1)

fig.update_layout(width=1800, title_text=fileName)
fig.show()
