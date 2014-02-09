<?php

require_once( 'common.php' );

$term = isset ( $_GET['term'] ) ? $_GET['term'] : '';

if ( '' === $term ) {
	page_header( 'Search ' );
	?>
	<form method="get">
		<p>To search for an actor or movie, input your search term before. Multiple terms (e.g. "a b") will be treated as "a AND b".</p>
		<input type="text" name="term">
		<input type="submit" value="Search">
	</form>
	<?php
	page_footer();
} else {
	page_header( 'Search Results for ' . $term );

	$term_list = explode( ' ', strtolower( $term ) );

	$actor_sql = 'SELECT Actor.id, CONCAT(Actor.first, " ", Actor.last) as Name FROM Actor WHERE 1';
	$movie_sql = 'SELECT Movie.id, Movie.title FROM Movie WHERE 1';

	// No PDO bindings here because of some wonkiness concerning bindings that are not surrounded by whitespace
	foreach ( $term_list as $individual_term ) {
		$actor_sql .= ' AND ( LOWER(Actor.first) LIKE "%' . $individual_term . '%" OR LOWER(Actor.last) LIKE "%' . $individual_term . '%" )';
		$movie_sql .= ' AND LOWER(Movie.title) LIKE "%' . $individual_term . '%"';
	}

	$dbh = get_db_handle();

	$stmt = $dbh->prepare( $actor_sql );
	$stmt->execute();
	$actors = $stmt->fetchAll( PDO::FETCH_ASSOC );

	$stmt = $dbh->prepare( $movie_sql );
	$stmt->execute();
	$movies = $stmt->fetchAll( PDO::FETCH_ASSOC );

	echo '<div><h2>Actor Results for <em>' . implode( $term_list, ' AND ' ) . '</em></h2>';
	echo render_table( $actors, 'person-view.php', 'id', 'Name', '', false ) . '</div>';

	echo '<div><h2>Movie Results for <em>' . implode( $term_list, ' AND ' ) . '</em></h2>';
	echo render_table( $movies, 'movie-view.php', 'id', 'title', '', false ) . '</div>';

	page_footer();
}