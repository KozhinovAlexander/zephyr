
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $script_dir/../.zephyr_venv/bin/activate


# Put your board under test to this list
board_list=(
	# "nucleo_h745zi_q/stm32h745xx/m7"
	"b_g431b_esc1/stm32g431xx"
)

# The apps are defined for this application
app_list=(
	"tests/drivers/interrupt_controller/intc_exti_stm32"
	# "tests/drivers/gpio/gpio_basic_api"
	# "tests/drivers/rtc/rtc_api"
)

sleep_time_sec_list=(
	1
	20
	60
)

script_path="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Iterate over boards:
for board in "${board_list[@]}"; do
	# Iterate over apps:
	for i in "${!app_list[@]}"; do
		app="${app_list[$i]}"
		app_abs_path="$script_path/$app"
		build_dir="$app_abs_path/build/$board"
		echo "--> use build directory: $build_dir"

		west build -p auto -d $build_dir -b $board $app_abs_path
		[ $? -ne 0 ] && exit 1;

		west flash -d $build_dir
		[ $? -ne 0 ] && exit 1;

		echo "--> wait ${sleep_time_sec_list[$i]} sec. for $board $app to finish..."
		sleep ${sleep_time_sec_list[$i]}
		done
done

exit 0;
