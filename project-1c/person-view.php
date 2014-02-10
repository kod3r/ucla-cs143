<?php

require_once('common.php');

$id = $_GET['id'] ? $_GET['id'] : '0';

$actor_sql = 'SELECT last, first, sex, dob, dod
	FROM Actor
	WHERE id = :id
	LIMIT 1
';

$director_sql = 'SELECT last, first, dob, dod
	FROM Director
	WHERE id = :id
	LIMIT 1
';

$dbh = get_db_handle();
$sth = $dbh->prepare( $actor_sql );
$sth->execute( array( ':id' => $id ) );
$actor = $sth->fetch( PDO::FETCH_ASSOC );

// Check if person is listed as a Director
if ( empty( $actor ) ) {
	$sth = $dbh->prepare( $director_sql );
	$sth->execute( array( ':id' => $id ) );
	$actor = $sth->fetch( PDO::FETCH_ASSOC );
}

if ( empty( $actor ) ) {
	error_404();
}

$acted = 'SELECT Movie.id as id, title as Title, role as Role, year as Year, rating as `Rating`, company as `Producing Company`
	FROM Movie
	JOIN MovieActor ON MovieActor.mid = Movie.id
	WHERE MovieActor.aid = :id
	ORDER BY year, title
';

$sth = $dbh->prepare( $acted );
$sth->execute( array( ':id' => $id ) );
$acted = $sth->fetchAll( PDO::FETCH_ASSOC );

$directed_sql = 'SELECT Movie.id as id, title as Title, year as Year, rating as `Rating`, company as `Producing Company`
	FROM Movie
	JOIN MovieDirector ON MovieDirector.mid = Movie.id
	JOIN Director ON MovieDirector.did = Director.id
	WHERE Director.id = :id
	ORDER BY year, title
';

$sth = $dbh->prepare( $directed_sql );
$sth->execute( array( ':id' => $id ) );
$directed = $sth->fetchAll( PDO::FETCH_ASSOC );

page_header( $actor['first'] . ' ' . $actor['last'] );
?>
		<p>
			<strong><?php echo $actor['first'] . ' ' . $actor['last']; ?></strong>
			<br>
			<?php if ( !empty( $actor['sex'] ) ) echo ucfirst( $actor['sex'] ) . '<br>'; ?>
			<?php echo $actor['dob'] ? 'Born: ' . date( 'F j, Y', strtotime( $actor['dob'] ) ) . '<br>' : ''; ?>
			<?php echo $actor['dod'] ? 'Died: ' . date( 'F j, Y', strtotime( $actor['dod'] ) ) . '<br>' : ''; ?>
		</p>

		<?php echo render_table( $directed, MOVIE_VIEW, 'id', 'Title', 'Films Directed' ) . '<br>'; ?>
		<?php echo render_table( $acted, MOVIE_VIEW, 'id', 'Title', 'Filmography' ); ?>
<?php page_footer(); ?>