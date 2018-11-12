-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_twkb" to load this file. \quit




CREATE OR REPLACE FUNCTION gunzip_text(bytea) RETURNS text
  AS 'MODULE_PATHNAME', 'gunzip_text'
  LANGUAGE C IMMUTABLE;
  
CREATE OR REPLACE FUNCTION gunzip_bytea(bytea) RETURNS bytea
  AS 'MODULE_PATHNAME', 'gunzip_bytea'
  LANGUAGE C IMMUTABLE;
  
CREATE OR REPLACE FUNCTION gunzip(bytea) RETURNS text
  AS 'MODULE_PATHNAME', 'gunzip_text'
  LANGUAGE C IMMUTABLE;
  
  
  
