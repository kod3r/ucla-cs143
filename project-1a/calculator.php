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
	$acceptable_chars = '/^[0-9\.\-\+\/\* ]+$/';
	if ( ! preg_match( $acceptable_chars, $expr ) ) {
		return false;
	}

	$expr = str_replace( ' ', '', $expr );

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
	$postfix_expr = '';
	$stack = array();
	$operators = array( '/', '*', '+', '-' );
	for( $i = 0; $i < strlen( $infix ); $i++ ) {
		$char = $infix[$i];

		if ( in_array( $char, $operators ) ) {
			if ( 0 == sizeof ( $stack ) ) {
				$stack[] = $char;
			}
			else {
				while ( sizeof( $stack ) > 0 && higher_precedence( $char, $stack[sizeof( $stack ) - 1] ) ) {
					$postfix_expr .= array_pop( $stack );
				}
				$stack[] = $char;
			}
		} else if ( is_numeric( $char ) || '.' == $char ) {
			// If it isn't a space, it must be a number
			$postfix_expr .= $char;
		}
	}
	$postfix_expr .= implode( $stack, '' );
	return $postfix_expr;
}

function evaluate_postfix_expression( $expr ) {
	$stack = array();
	$operators = array( '/', '*', '+', '-' );
	for ( $i = 0; $i < sizeof( $expr ); $i++ ) {
		$char = $expr[$i];
		if ( ! in_array( $char, $operators ) ) {
			$stack[] = $char;
		} else {
			switch ( $char ) {
				case '+':
					$result = $stack[sizeof( $stack )] + $stack[sizeof( $stack ) - 1];
					break;
				case '-':
					$result = $stack[sizeof( $stack )] - $stack[sizeof( $stack ) - 1];
					break;
				case '/':
					$result = $stack[sizeof( $stack )] / $stack[sizeof( $stack ) - 1];
					break;
				case '*':
					$result = $stack[sizeof( $stack )] * $stack[sizeof( $stack ) - 1];
					break;
			}
			$stack = array_slice( $stack, 0, sizeof( $stack ) - 2 );
			$stack[] = $result;
		}
	}

	return $stack[0];
}

/**
 * Determine if a is an operand of higher or equal precedence than b
 */
function higher_precedence( $a, $b ) {
	switch ( $b ) {
		case '+':
		case '-':
			return false;
			break;
		// There can never be a higher precedence operator
		case '*':
		case '/':
			if ( $a == '+' || $a == '-')
				return true;
			else
				return false;
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
		echo 'Your result is: ' . evaluate_postfix_expression( infix_to_postfix( $expression ) );
	}
}

/** 
 * Finish up the "template"
 */
?>
	</body>
</html>