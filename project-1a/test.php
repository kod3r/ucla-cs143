<?php
/**
 * Some tests to ensure that things are functioning as expected.
 * This should NOT be submitted.
 */

print "Beginning tests...\n";
$fail = $pass = 0;

$tests = array(
	'2+2', // addition
	'9*8', // multiplication
	'144/12', // division
	'100-101', // substraction
	'10+-9', // negative numbers
	'.1+.25', // fractional expressions
	'49', // identity
	'2*3*-4', // multiple operators
	'3/2+1/3', // fractions
	'100-100/100', // order of operations
	'-2/-3', // multiple negatives, fractional result
	'1/2+9/8*.5', // fraction and decimals
);

// Load the actual functions...
ob_start();
include 'calculator.php';
ob_end_clean();

foreach ( $tests as $test ) {
	eval( '$expect = ' . $test . ';' );
	print "Testing $test = $expect ... ";
	$result = evaluate_postfix_expression( infix_to_postfix( $test ) );

	if ( $result == $expect ) {
		$pass++;
		print "Passed!\n";
	} else {
		print "Got $result - FAIL\n";
		$fail++;
	}
}

// Now, make sure desired failures fail
$failures = array(
	'one/zero', // written numbers
);

foreach ( $failures as $failure ) {
	print "Testing $failure ... ";

	if ( ! is_valid_expression( $failure ) ) {
		$pass++;
		print "Detected as invalid. Passed!\n";
	} else {
		$fail++;
		print "Detected as valid. FAIL\n";
	}
}

print "Tests complete! $fail failures | $pass successes\n";