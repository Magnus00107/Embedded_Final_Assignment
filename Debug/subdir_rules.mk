################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Users/magnusmeldgaard/workspace_v12/Poster Assignment" --include_path="/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/inc" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/port/TivaM4" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

heap_2.obj: /Users/magnusmeldgaard/Desktop/Robotteknologi/4.\ Semester/Indlejret\ Programmering/Final\ assignment/frt10/port/MemMang/heap_2.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Users/magnusmeldgaard/workspace_v12/Poster Assignment" --include_path="/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/inc" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/port/TivaM4" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="heap_2.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

list.obj: /Users/magnusmeldgaard/Desktop/Robotteknologi/4.\ Semester/Indlejret\ Programmering/Final\ assignment/frt10/src/list.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Users/magnusmeldgaard/workspace_v12/Poster Assignment" --include_path="/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/inc" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/port/TivaM4" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="list.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

port.obj: /Users/magnusmeldgaard/Desktop/Robotteknologi/4.\ Semester/Indlejret\ Programmering/Final\ assignment/frt10/port/TivaM4/port.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Users/magnusmeldgaard/workspace_v12/Poster Assignment" --include_path="/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/inc" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/port/TivaM4" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="port.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

portasm.obj: /Users/magnusmeldgaard/Desktop/Robotteknologi/4.\ Semester/Indlejret\ Programmering/Final\ assignment/frt10/port/TivaM4/portasm.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Users/magnusmeldgaard/workspace_v12/Poster Assignment" --include_path="/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/inc" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/port/TivaM4" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="portasm.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

queue.obj: /Users/magnusmeldgaard/Desktop/Robotteknologi/4.\ Semester/Indlejret\ Programmering/Final\ assignment/frt10/src/queue.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Users/magnusmeldgaard/workspace_v12/Poster Assignment" --include_path="/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/inc" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/port/TivaM4" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="queue.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_frt.obj: /Users/magnusmeldgaard/Desktop/Robotteknologi/4.\ Semester/Indlejret\ Programmering/Final\ assignment/frt10/port/TivaM4/startup_frt.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Users/magnusmeldgaard/workspace_v12/Poster Assignment" --include_path="/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/inc" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/port/TivaM4" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="startup_frt.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

systick_frt.obj: /Users/magnusmeldgaard/Desktop/Robotteknologi/4.\ Semester/Indlejret\ Programmering/Final\ assignment/frt10/port/TivaM4/systick_frt.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Users/magnusmeldgaard/workspace_v12/Poster Assignment" --include_path="/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/inc" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/port/TivaM4" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="systick_frt.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

tasks.obj: /Users/magnusmeldgaard/Desktop/Robotteknologi/4.\ Semester/Indlejret\ Programmering/Final\ assignment/frt10/src/tasks.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Users/magnusmeldgaard/workspace_v12/Poster Assignment" --include_path="/Applications/ti/ccs1281/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/inc" --include_path="/Users/magnusmeldgaard/Desktop/Robotteknologi/4. Semester/Indlejret Programmering/Final assignment/frt10/port/TivaM4" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="tasks.d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


