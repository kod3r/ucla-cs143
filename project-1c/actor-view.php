<?php

require_once('common.php');

$id = $_GET['id'] ? $_GET['id'] : '0';

$actor_sql = 'SELECT last, first, sex, dob, dod
	FROM Actor
	WHERE id = :id
	LIMIT 1
';

$dbh = get_db_handle();
$sth = $dbh->prepare( $actor_sql );
$sth->execute( array( ':id' => $id ) );
$actor = $sth->fetch( PDO::FETCH_ASSOC );

if ( empty( $actor ) ) {
	error_404();
}

$movies_sql = 'SELECT title as Title, role as Role, year as Year, rating as `Rating`, company as `Producing Company`
	FROM Movie
	JOIN MovieActor ON MovieActor.mid = Movie.id
	JOIN Actor ON MovieActor.aid = Actor.id
	WHERE Actor.id = :id
	ORDER BY year, title
';

$sth = $dbh->prepare( $movies_sql );
$sth->execute( array( ':id' => $id ) );
$movies = $sth->fetchAll( PDO::FETCH_ASSOC );

$movies_html = '';

// @TODO: hyperlink movies to their respective page
if ( !empty( $movies ) ) {
	$col_names = array_keys( $movies[0] );
	$movies_html = '<table border="1"><tr><th colspan="' . sizeof( $col_names ) . '">Filmography</th></tr>';

	$movies_html .= '<tr>';
	foreach ( $col_names as $col ) {
		$movies_html .= "<td><strong>$col</strong></td>";
	}
	$movies_html .= '</tr>';

	foreach ( $movies as $m ) {
		$movies_html .= '<tr>';
		foreach ( $m as $col ) {
			$movies_html .= "<td>$col</td>";
		}
		$movies_html .= '</tr>';
	}

	$movies_html .= '</table>';
}

?>
<!DOCTYPE html>
<html>
	<head>
		<title>CS143 Project 1C - Movie Database</title>
	</head>
	<body>
		<p>
			<?php echo $actor['first'] . ' ' . $actor['last']; ?>
			<br>
			<?php echo $actor['sex']; ?>
			<br>
			<?php echo $actor['dob'] ? 'Born: ' . date( 'F j, Y', strtotime( $actor['dob'] ) ) . '<br>' : ''; ?>
			<?php echo $actor['dod'] ? 'Died: ' . date( 'F j, Y', strtotime( $actor['dod'] ) ) . '<br>' : ''; ?>
		</p>

		<?php echo $movies_html; ?>
	</body>
</html>


