LOAD DATA INFILE '$(HOME)/data/actor1.del' INTO TABLE `Actor`;
LOAD DATA INFILE '$(HOME)/data/actor2.del' INTO TABLE `Actor`;
LOAD DATA INFILE '$(HOME)/data/actor3.del' INTO TABLE `Actor`;

LOAD DATA INFILE '$(HOME)/data/director.del' INTO TABLE `Director`;

LOAD DATA INFILE '$(HOME)/data/movie.del' INTO TABLE `Movie`;

LOAD DATA INFILE '$(HOME)/data/movieactor1.del' INTO TABLE `MovieActor`;
LOAD DATA INFILE '$(HOME)/data/movieactor2.del' INTO TABLE `MovieActor`;

LOAD DATA INFILE '$(HOME)/data/moviedirector.del' INTO TABLE `MovieDirector`;

LOAD DATA INFILE '$(HOME)/data/moviegenre.del' INTO TABLE `MovieGenre`;