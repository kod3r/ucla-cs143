<?php

require_once('common.php');

$id = $_GET['id'] ? $_GET['id'] : '0';

$movie_sql = 'SELECT title, year, rating, company
	FROM Movie
	WHERE id = :id
	LIMIT 1
';

$dbh = get_db_handle();
$sth = $dbh->prepare( $movie_sql );
$sth->execute( array( ':id' => $id ) );
$movie = $sth->fetch( PDO::FETCH_ASSOC );

$cast_sql = 'SELECT Actor.id, CONCAT(Actor.first, " ", Actor.last) as Name, MovieActor.role as Role
	FROM Actor
	JOIN MovieActor ON MovieActor.aid = Actor.id
	WHERE MovieActor.mid = :id
';

$director_sql ='SELECT Director.id, CONCAT(Director.first, " ", Director.last) as Name
	FROM Director
	JOIN MovieDirector ON MovieDirector.did = Director.id
	WHERE MovieDirector.mid = :id
';

$genre_sql = 'SELECT DISTINCT(genre)
	FROM MovieGenre
	WHERE mid = :id
';

$avg_rating_sql = 'SELECT AVG(rating)
	FROM Review
	WHERE mid = :id
';

$comments_sql = 'SELECT name, CONCAT(rating, "/5") as rating, comment
	FROM Review
	WHERE mid = :id
	ORDER BY time DESC
';

$sth = $dbh->prepare( $cast_sql );
$sth->execute( array( ':id' => $id ) );
$cast = $sth->fetchAll( PDO::FETCH_ASSOC );

$sth = $dbh->prepare( $director_sql );
$sth->execute( array( ':id' => $id ) );
$directors = $sth->fetchAll( PDO::FETCH_ASSOC );

$sth = $dbh->prepare( $genre_sql );
$sth->execute( array( ':id' => $id ) );
$genres = $sth->fetchAll( PDO::FETCH_COLUMN, 0 );

$sth = $dbh->prepare( $avg_rating_sql );
$sth->execute( array( ':id' => $id ) );
$avg_rating = $sth->fetch( PDO::FETCH_COLUMN, 0 );
$avg_rating = $avg_rating ? number_format( (float)$avg_rating, 1) : '--';

$sth = $dbh->prepare( $comments_sql );
$sth->execute( array( ':id' => $id ) );
$comments = $sth->fetchAll( PDO::FETCH_ASSOC );

if ( sizeof( $comments ) > 0 ) {
	$comments_html = '';
	foreach ( $comments as $c ) {
		$comments_html .= '<div style="border: 4px double gray;"><p style="margin: 5px">';
		$comments_html .= 'Name: '   . $c['name']   . '<br>';
		$comments_html .= 'Rating: ' . $c['rating'] . '<br>';
		$comments_html .= $c['comment'];
		$comments_html .= '</p></div>';
	}
}
page_header( $movie['title'] );
?>
		<p>
			<strong><?php echo $movie['title'] ?></strong>, <em><?php echo $movie['year']; ?></em>
			<br>
			Rating: <?php echo $movie['rating']; ?>
			<br>
			Produced by: <?php echo $movie['company']; ?>
			<br>
			Tagged genres: <?php echo implode( ', ', $genres ); ?>
			<br>
			Average Rating: <?php echo $avg_rating; ?>/5
		</p>

		<?php echo render_table( $directors, PERSON_VIEW, 'id', 'Name', 'Directed By', false ); ?>
		<br>
		<?php echo render_table( $cast, PERSON_VIEW, 'id', 'Name', 'Film Cast' ); ?>
		<br>
		<div style="width: 45%;">
			<strong>User reviews</strong>
			<br>
			<?php echo hyperlink( REVIEW_FORM, $id, 'Add review'); ?>
			<?php echo $comments_html; ?>
		</div>
<?php page_footer(); ?>