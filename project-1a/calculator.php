<?php
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

}

/** 
 * Finish up the "template"
 */
?>
	</body>
</html>