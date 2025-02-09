# RISC-V

Use the "make" command to compile the necessary files

The functionality of the commands can be verified by individual command tests using the following command:

../../rv64sim/rv64sim < tests/command_tests/command_test_m.cmd

Compare your output with the expected results:

../../rv64sim/rv64sim < tests/command_tests/command_test_m.cmd | diff --strip-trailing-cr - tests/command_tests/expected/command_test_m.log

Harness tests are used to verify basic instruction execution. Run them similarly to command tests:

../../rv64sim/rv64sim < tests/harness_tests/harness_test_addi.cmd

Instruction tests are more comprehensive and test the full instruction set. Use the provided scripts to run them:

./tests/instruction_tests/run_test instruction_test_add

To run all instruction tests:

./tests/instruction_tests/run_instruction_tests

Compiled tests are C programs compiled to RISC-V assembly. Run them using:

./tests/compiled_tests/run_test compiled_test_fib

To run all compiled tests:

./tests/compiled_tests/run_compiled_tests

Use the -v option to enable verbose output, which can help with debugging:

RV64SIM_FLAGS='-v' ./tests/instruction_tests/run_test instruction_test_add
   
The provided benchmark can help gauge performance:

time ./tests/compiled_tests/run_compiled_tests
