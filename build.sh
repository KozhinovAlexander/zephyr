#!/usr/bin/env bash
# Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $script_dir/../.zephyr_venv/bin/activate

export ZEPHYR_BASE="${script_dir}"

# ./scripts/checkpatch.pl --git HEAD-2
# ./scripts/ci/check_compliance.py

# Put your board under test to this list (default all boards)
board_list=(
    "nucleo_g474re/stm32g474xx"
    # "nucleo_h745zi_q/stm32h745xx/m7"
    # "nucleo_h745zi_q/stm32h745xx/m4"
)

# The apps are defined for this application
app_list=(
    "tests/drivers/cordic/cordic_api"
    # "samples/drivers/cordic"
)

# This array shall have same size as app_list
sleep_time_sec_list=(
    2
    2
    2
)

script_path="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Iterate over boards:
for board in "${board_list[@]}"; do
    # Iterate over apps:
    for i in "${!app_list[@]}"; do
        app="${app_list[$i]}"
        app_abs_path="$script_path/$app"
        safe_board_name="${board//\//_}"
        build_dir="$app_abs_path/build/$safe_board_name"
        echo -e "\n--> use build directory: $build_dir"

        # Test app starts with "tests"
        if [[ "$app" == *"tests/"* ]]; then
            # Twister targets shall be built slightly different way
            # NOTE: Ensure the device is not attached to minicom or screen, or else
            west twister -p $board -T $app_abs_path --vendor st \
                --device-testing \
                --device-serial-baud 115200 \
                --device-serial "/dev/ttyACM0" -v
        else
            west build -p auto -d $build_dir -b $board $app_abs_path
            [ $? -ne 0 ] && exit $?;
            west flash -d $build_dir
            [ $? -ne 0 ] && exit $?;
        fi
        # echo "--> wait ${sleep_time_sec_list[$i]} sec. for $board $app to finish..."
        # sleep ${sleep_time_sec_list[$i]}
    done
done

exit 0;
