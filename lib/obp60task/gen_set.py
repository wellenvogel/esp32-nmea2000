#!/usr/bin/env python3
# A tool to generate that part of config.json  that deals with pages and fields.
#
#Usage: 1. modify this script (e.g.add a page, change number of fields, etc.)
#       2. Delete all lines from config.json from the curly backet before "name": "page1type"  to o the end of the file (as of today, delete from line 917 to the end of the File)
#       3. run ./gen_set.py >> config.json

import json

# List of all pages and the number of parameters they expect.
no_of_fields_per_page = {
    "Wind": 0,
    "XTETrack": 0,
    "Battery2": 0,
    "Battery": 0,
    "BME280": 0,
    "Clock": 0,
    "DST810": 0,
    "Fluid": 1,
    "FourValues2": 4,
    "FourValues": 4,
    "Generator": 0,
    "KeelPosition": 0,
    "OneValue": 1,
    "RollPitch": 2,
    "RudderPosition": 0,
    "Solar": 0,
    "ThreeValues": 3,
    "TwoValues": 2,
    "Voltage": 0,
    "White": 0,
    "WindRose": 0,
    "WindRoseFlex": 6,
    "SixValues" : 6,
}

# No changes needed beyond this point
# max number of pages supported by OBP60
no_of_pages = 10
# Default selection for each page
default_pages = [
    "Voltage",
    "WindRose",
    "OneValue",
    "TwoValues",
    "ThreeValues",
    "FourValues",
    "FourValues2",
    "Clock",
    "RollPitch",
    "Battery2",
]
numbers = [
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine",
    "ten",
]
pages = sorted(no_of_fields_per_page.keys())
max_no_of_fields_per_page = max(no_of_fields_per_page.values())

output = []

for page_no in range(1, no_of_pages + 1):
    page_data = {
        "name": f"page{page_no}type",
        "label": "Type",
        "type": "list",
        "default": default_pages[page_no - 1],
        "description": f"Type of page for page {page_no}",
        "list": pages,
        "category": f"OBP60 Page {page_no}",
        "capabilities": {"obp60": "true"},
        "condition": [{"visiblePages": vp} for vp in range(page_no, no_of_pages + 1)],
        #"fields": [],
    }
    output.append(page_data)

    for field_no in range(1, max_no_of_fields_per_page + 1):
        field_data = {
            "name": f"page{page_no}value{field_no}",
            "label": f"Field {field_no}",
            "type": "boatData",
            "default": "",
            "description": f"The display for field {numbers[field_no - 1]}",
            "category": f"OBP60 Page {page_no}",
            "capabilities": {"obp60": "true"},
            "condition": [
                {f"page{page_no}type": page}
                for page in pages
                if no_of_fields_per_page[page] >= field_no
            ],
        }
        output.append(field_data)

    fluid_data ={
        "name": f"page{page_no}fluid",
        "label": "Fluid type",
        "type": "list",
        "default": "0",
        "list": [
        {"l":"Fuel (0)","v":"0"},
        {"l":"Water (1)","v":"1"},
        {"l":"Gray Water (2)","v":"2"},
        {"l":"Live Well (3)","v":"3"},
        {"l":"Oil (4)","v":"4"},
        {"l":"Black Water (5)","v":"5"},
        {"l":"Fuel Gasoline (6)","v":"6"}
        ],
        "description": "Fluid type in tank",
        "category": f"OBP60 Page {page_no}",
        "capabilities": {
        "obp60":"true"
        },
        "condition":[{f"page{page_no}type":"Fluid"}]
        } 
    output.append(fluid_data)

json_output = json.dumps(output, indent=4)
# print omitting first and last line containing [ ] of JSON array
print(json_output[1:])
# print(",")