CREATE DATABASE IF NOT EXISTS bbn_actor_db;

USE bbn_actor_db;

CREATE TABLE sources (
  source_id int(11) NOT NULL auto_increment,
  source_name varchar(100) NOT NULL,
  PRIMARY KEY  (source_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE actors (
  actor_id bigint NOT NULL auto_increment,
  canonical_name varchar(200) NOT NULL,
  ace_entity_type char(3) NOT NULL default 'UNK',
  ace_entity_subtype varchar(50) NOT NULL default 'unknown',
  iso_code varchar(3) default NULL,
  PRIMARY KEY  (actor_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE actor_strings (
  item_id bigint NOT NULL auto_increment,
  actor_id bigint NOT NULL,
  string text NOT NULL,
  confidence float NOT NULL,
  source_id int(11) NOT NULL,
  acronym boolean NOT NULL,
   primary key (item_id),
   #key too long because of 'string'
   #unique key unique_actor_string (actor_id,string,source_id),
   foreign key (actor_id) references actors(actor_id),
   foreign key (source_id) references sources(source_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE descriptions (
  item_id bigint NOT NULL auto_increment,
  actor_id bigint NOT NULL,
  string text NOT NULL,
  count int(11) NOT NULL default '0',
  source_id int(11) NOT NULL,
   primary key (item_id),
   #unique key unique_actor_description (actor_id,string,source_id),
   foreign key (actor_id) references actors(actor_id),
   foreign key (source_id) references sources(source_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE actor_sources (
  item_id bigint NOT NULL auto_increment,
  actor_id bigint NOT NULL,
  source_id int(11) NOT NULL,
  original_source_element varchar(250) default NULL collate utf8_bin, 
  #changed original_source_element from 1000 to 250 since it has to be in the composite key, and since its length in DBPedia DB is also 250
  match_mechanism varchar(50) DEFAULT 'edit_distance_with_thresh=0', 
  match_confidence float NOT NULL,
  primary key (item_id),
  unique key unique_actor_source_mapping (actor_id,source_id,original_source_element,match_mechanism),
  foreign key (actor_id) references actors(actor_id),
  foreign key (source_id) references sources(source_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
Skip for now. Let there be no concept of relation-type, as was the case originally, and is the case with ICEWS db.
CREATE TABLE Relation_Names(
  relation_id bigint NOT NULL auto_increment,
  relation_name varchar(100) default 'unspecified',
  ace_relation_type varchar(50) NOT NULL default 'unknown',
  ace_relation_subtype varchar(50) NOT NULL default 'unknown',
  source_id int(11) NOT NULL,
  primary key (relation_id),
  unique key 'unique_relation' (ace_relation_type,ace_relation_subtype),
  foreign key (source_id) references sources(source_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
*/
/*
CREATE TABLE links (
  link_id bigint NOT NULL auto_increment,
  left_actor_id bigint NOT NULL,
  right_actor_id bigint NOT NULL,
  relation_id bigint NOT NULL,
  confidence float NOT NULL,
  source_id int(11) NOT NULL,
  primary key (link_id),
  unique key unique_link (left_actor_id,right_actor_id,relation_id,source_id),
  foreign key (left_actor_id) references actors(actor_id),
  foreign key (right_actor_id) references actors(actor_id),
  foreign key (relation_id) references Relation_Names(relation_id),
  foreign key (source_id) references sources(source_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
*/

CREATE TABLE link_types (
  link_type_id bigint NOT NULL auto_increment,
  link_type varchar(100) NOT NULL,
  PRIMARY KEY  (link_type_id),
  unique key unique_link_type (link_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE links (
  link_id bigint NOT NULL auto_increment,
  left_actor_id bigint NOT NULL,
  right_actor_id bigint NOT NULL,
  link_type_id bigint NOT NULL,
  start_date date,
  end_date date,
  primary key (link_id),
  unique key unique_link (left_actor_id,right_actor_id,link_type_id,start_date,end_date),
  foreign key (left_actor_id) references actors(actor_id),
  foreign key (right_actor_id) references actors(actor_id),
  foreign key (link_type_id) references link_types(link_type_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE link_sources (
  item_id bigint NOT NULL auto_increment,
  link_id bigint NOT NULL,
  source_id int(11) NOT NULL,
  confidence float NOT NULL,
  primary key (item_id),
  unique key unique_link_source_mapping (link_id,source_id),
  foreign key (link_id) references links(link_id),
  foreign key (source_id) references sources(source_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE sectors (
  sector_id bigint NOT NULL auto_increment,
  #changed from varchar(1000) to varchar(100) for similar reasons as above
  sector_name varchar(100) NOT NULL default 'unspecified',
  PRIMARY KEY  (sector_id),
  unique key unique_sector_name (sector_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE sector_sources (
  item_id bigint NOT NULL auto_increment,
  sector_id bigint NOT NULL,
  source_id int(11) NOT NULL,
  original_source_element varchar(250) default NULL collate utf8_bin,
  match_mechanism varchar(50) DEFAULT 'edit_distance_with_thresh=0',
  match_confidence float NOT NULL,
  primary key (item_id),
  unique key unique_sector_source_mapping (sector_id,source_id,original_source_element,match_mechanism),
  foreign key (sector_id) references sectors(sector_id),
  foreign key (source_id) references sources(source_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE actor_sector_links (
  item_id bigint NOT NULL auto_increment, 	
  actor_id bigint NOT NULL,
  sector_id bigint NOT NULL,
  start_date date,
  end_date date,
  primary key (item_id),
  unique key unique_actor_sector_relation (actor_id,sector_id,start_date,end_date),
  foreign key (actor_id) references actors(actor_id),
  foreign key (sector_id) references sectors(sector_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE actor_sector_link_sources (
  item_id bigint NOT NULL auto_increment,
  actor_sector_link_id bigint NOT NULL,
  confidence float NOT NULL,
  source_id int(11) NOT NULL,
  primary key (item_id),
  unique key unique_actor_sector_link_source_mapping (actor_sector_link_id,source_id),
  foreign key (actor_sector_link_id) references actor_sector_links(item_id),
  foreign key (source_id) references sources(source_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

# Additionally, insert default SOURCES into the SOURCES table:
INSERT INTO sources (source_name) values ('DBPedia');
INSERT INTO sources (source_name) values ('Freebase');
INSERT INTO sources (source_name) values ('W-ICEWS actor dictionary');

# Default link types
INSERT INTO link_types (link_type) values ('Affiliation');
INSERT INTO link_types (link_type) values ('Country');
