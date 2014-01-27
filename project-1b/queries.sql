-- All actors are joined to all movies they have acted in via the MovieActor table
-- then the results are filtered by Movie titles that match "Die Another Day".
-- Then the actor's first and last names are concatenated with a space and selected
-- as a renamed column
SELECT CONCAT(first, ' ', last) AS `Actors in "Die Another Day"`
FROM Actor
JOIN MovieActor ON Actor.id = MovieActor.aid
JOIN Movie on MovieActor.mid = Movie.id
WHERE Movie.title = 'Die Another Day'

-- All actors are joined to all movies they have acted in via the MovieActor table
-- and the results are grouped by actor ids. The results are then filtered to contain
-- only those actor ids where their associated Movie.id rows are greater than 1, and then
-- all these results are counted
SELECT COUNT(*)
FROM (
	SELECT Actor.id
	FROM Actor
	JOIN MovieActor ON Actor.id = MovieActor.aid
	GROUP BY Actor.id
	HAVING COUNT(MovieActor.aid) > 1
) as tmp

-- This query counts the number of directors who have had a role in their own movie
-- First we join together Actors and Directors to see who has had been both. Next we join
-- with MovieActor and MovieDirector to get all the movies an actor has acted in or directed.
-- Then we filter the results by those where the actor has had a role and directed, and these
-- results are then all counted
SELECT COUNT(*)
FROM (
	SELECT Actor.id
	FROM Actor
	JOIN Director on Actor.id = Director.id
	JOIN MovieActor on Actor.id = MovieActor.aid
	JOIN MovieDirector on Director.id = MovieDirector.did
	WHERE MovieActor.mid = MovieDirector.mid
	GROUP BY Actor.id
) as tmp