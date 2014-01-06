<?php
ini_set( 'display_errors', 1 );
error_reporting( E_ALL );
/**
 * Simple "Calculator"
 * For UCLA CS143, Winter 2014.
 * @author Phil Crumm <pcrumm@ucla.edu>
 * @author Ivan Petkov <ippetkov@ucla.edu>
 * @license Public domain
 */

/**
 * Some design notes:
 * The project requires that a single file only be submitted, so this
 * is a little bit ugly.
 */

/**
* "Utility" functions. These are not actually used more than once,
* but we separate them out for clarity.
*/
function is_valid_expression( $expr ) {
	$acceptable_chars = '/^[0-9\.\-\+\/\*]+$/';
	if ( ! preg_match( $acceptable_chars, $expr ) ) {
		return false;
	}

	// Permutations of consecutive +, /, or * are invalid, as is a - followed by another operator
	// An operator followed by a - is valid as it becomes a unary minus
	if ( preg_match( '/[\+\/\*]{2,}/', $expr ) || preg_match( '/\-[\+\/\*]/', $expr ) ) {
		return false;
	}

	// A decimal without leading or trailing numeric characters is illegal
	// Two decimal points can only be valid if an operator exists between them
	// A digit must appear before or after a decimal point
	if ( preg_match( '/[^0-9]\.[^0-9]/', $expr ) || preg_match( '/\.[^0-9]/', $expr ) || preg_match( '/\.[^\+\-\/\*]+\./', $expr ) ) {
		return false;
	}

	return true;
}

function infix_to_postfix ( $infix ) {
	$infix = str_replace( ' ', '', $infix );

	$postfix = array();
	$stack = array();
	$operators = array( '/', '*', '+', '-' );

	// stores the current operand value, e.g. 5
	// NULL distinguishes between uninitialized and the literal 0 number
	$current_operand = NULL;

	$unary_min = false;
	$is_fractional_part = false; // have we passed a decimal ('.') character yet?
	$next_decimal_power = 0; // if $current_operand is 5.03, $next_decimal_power should equal (-)3

	for( $i = 0; $i < strlen( $infix ); $i++ ) {
		$char = $infix[$i];

		// Process natural numbers one character at a time
		if ( is_numeric( $char ) ) {
			if ( $is_fractional_part ) {
				$current_operand += pow( 10, -$next_decimal_power ) * $char;
				$next_decimal_power += 1;
			} else {
				$current_operand = ($current_operand * 10) + $char;
			}

			continue;
		}

		// Hit a decimal character, following numbers are fractional part
		if ( '.' == $char ) {
			$is_fractional_part = true;
			$next_decimal_power = 0;
			continue;
		}

		// Found an operator
		if ( in_array( $char, $operators ) ) {
			// Check for unary minus and togle the flag
			// Note consecutive '-' operators are prefectly valid and anything after
			// the first should be treated as a unary minus
			if ( '-' == $char && ( 0 == $i || in_array( $infix[$i - 1], $operators ) ) ) {
				$unary_min = ! $unary_min;
				continue;
			}

			// Push the operand on the evaluation stack
			$postfix[] = $current_operand * ($unary_min ? -1 : 1);

			// Reset operand flags
			$current_operand = NULL;
			$unary_min = false;
			$is_fractional_part = false;
			$next_decimal_power = 0;

			while ( sizeof( $stack ) > 0 && higher_precedence( $stack[sizeof( $stack ) - 1], $char ) ) {
				$postfix[] = array_pop( $stack );
			}

			$stack[] = $char;
		}
	}

	// If we hit the end of the string push the last operand
	if ( NULL !== $current_operand ) {
		$postfix[] = $current_operand * ($unary_min ? -1 : 1);
	}

	while ( sizeof( $stack ) > 0 ) {
		$postfix[] = array_pop( $stack );
	}

	return $postfix;
}

function evaluate_postfix_expression( $expr ) {
	$stack = array();
	$operators = array( '/', '*', '+', '-' );
	for ( $i = 0; $i < sizeof( $expr ); $i++ ) {
		$char = $expr[$i];
		if ( ! in_array( (string)$char, $operators ) ) {
			$stack[] = $char;
		} else {
			$oper2 = (float)array_pop( $stack );
			$oper1 = (float)array_pop( $stack );

			switch ( $char ) {
				case '+':
					$stack[] = $oper1 + $oper2;
					break;
				case '-':
					$stack[] = $oper1 - $oper2;
					break;
				case '/':
					if ( 0 == $oper2 )
						return 'Error: Divide by zero exception.';

					$stack[] = $oper1 / $oper2;
					break;
				case '*':
					$stack[] = $oper1 * $oper2;
					break;
			}
		}
	}

	return $stack[0];
}

/**
 * Determine if a is an operand of higher precedence than b
 */
function higher_precedence( $a, $b ) {
	switch ( $b ) {
		case '+':
		case '-':
			return true;
			break;
		// There can never be a higher precedence operator
		case '*':
		case '/':
			if ( $a == '+' || $a == '-')
				return false;
			else
				return true;
			break;
	}
}

/**
 * Some common HTML templating...
 */
?>
<html>
	<head>
		<title>Calculator</title>
	</head>
	<body>
<?php
$expression = empty( $_GET['expression'] ) ? '' : $_GET['expression'];


/**
 * Display the "calculator" form. We do this on every page.
 */
?>
<form action="" method="get">
	<label for="expression">Your Calculator Expression:</label>
	<input type="text" name="expression">
	<input type="submit" name="submit" value="Calculator">
</form>
<p>Some rules:
	<ul>
		<li>Expressions will be computed following the order of operations.</li>
		<li>Only the following operators are supported: *, +, -, /. Parenthesis are not supported.</li>
		<li>You may enter whole numbers (e.g. 1) or decimal (floating point) numbers (e.g. 1.2).</li>
		<li>Negative numbers are acceptable.</li>
		<li>Non-numeric characters, other than the operators above, are not allowed.</li>
		<li>Spaces will be ignored.</li>
	</ul>
</p>
<?php
/**
 * Compute the result, if an expression is provided. If there's an error,
 * simply display it.
 */
if ( '' != $expression ) {
	echo '<h2>Results</h2>';

	if ( ! is_valid_expression( $expression ) ) {
		echo '<p><strong>Sorry, your expression is invalid!</strong> Please try again.</p>';
	} else {
		$result = evaluate_postfix_expression( infix_to_postfix( $expression ) );

		if ( is_numeric( $result ) )
			$result = 'Your result is: ' . $result;
		else if ( ! is_string( $result ) )
			$result = "An error occured.";

		echo $result;
	}
}

/** 
 * Finish up the "template"
 */
?>
	</body>
</html>