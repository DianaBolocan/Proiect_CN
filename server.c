#define PORT 11111
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include <ctype.h>

extern int errno;

int getPrivilege(char* username){
  int privilege = 1;
  char *db_name = "users.sqlite";
  sqlite3 *db;
  int rc;

  rc = sqlite3_open(db_name,&db);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open users.sqlite.\n");
	fflush(stdout);
	return 0;
  }

  const char* query = "select vote_perm from users where username like ?";
  sqlite3_stmt *statement;

  rc = sqlite3_prepare_v2(db,query,-1,&statement,NULL);
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return 0;
  }

  rc = sqlite3_bind_text(statement,1,username,strlen(username),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind username.\n");
	fflush(stdout);
	return 0;
  }

  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }else if (rc == SQLITE_ROW){
	privilege = (int)sqlite3_column_int(statement,0);
  }

  sqlite3_close(db);
  return privilege;
}

int addSong(char* artistName,char* songName,char* subgenre){
  int index = 1;
  char *db_name = "songs.sqlite";
  sqlite3 *db;
  int rc;
  char *artist = artistName;
  char *song = songName;

  for(int i=0;i < strlen(artist);i++)
	artist[i] = tolower(artist[i]);
  for(int i=0;i < strlen(song);i++)
	song[i] = tolower(song[i]);
  for(int i=0;i < strlen(subgenre);i++)
	subgenre[i] = tolower(subgenre[i]);

  rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }

  //check duplicate song
  const char* searchQuery = "select artist_name,song_name from songs where lower(artist_name) like ? and lower(song_name) like ?";
  sqlite3_stmt *searchStmt;

  rc = sqlite3_prepare_v2(db,searchQuery,-1,&searchStmt,NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return 0;
  }

  //bind first value
  rc = sqlite3_bind_text(searchStmt,1,artist,strlen(artist),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind artist_name.\n");
	fflush(stdout);
	return 0;
  }

  //bind second value
  rc = sqlite3_bind_text(searchStmt,2,song,strlen(song),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind song_name.\n");
	fflush(stdout);
	return 0;
  }

  rc = sqlite3_step(searchStmt);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }else if (rc == SQLITE_ROW){
	sqlite3_close(db);	
	printf("[SQLITE]: Already in database.\n");
	fflush(stdout);	
	return 0;
  }

  sqlite3_finalize(searchStmt);
	
  //get all ids of songs
  const char* query = "select id_song from songs order by id_song";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db,query,-1,&statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return 0;
  }
  rc = sqlite3_step(statement);
  char idSong[5];
  int indexCheck;
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't execute id statement.\n");
	fflush(stdout);
	return 0;
  }else if(rc == SQLITE_ROW) {
	indexCheck = sqlite3_column_int(statement,0);
	if(indexCheck == index)
		index++;
  }
	  
  while(sqlite3_step(statement) == SQLITE_ROW){
	indexCheck = sqlite3_column_int(statement,0);
	if(indexCheck != index) {break;}
	index++;
  }

  sqlite3_finalize(statement);
  sqlite3_close(db);

  char *db_name3 = "subgenres.sqlite";
  sqlite3 *db3;

  rc = sqlite3_open(db_name3, &db3);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }

  //get id subgenre
  const char* getId = "select id_subgenre from subgenres where lower(subgenre_name) like ?";
  sqlite3_stmt *getIdStmt;

  rc = sqlite3_prepare_v2(db3,getId,-1,&getIdStmt,NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return 0;
  }

  //bind first value
  rc = sqlite3_bind_text(getIdStmt,1,subgenre,strlen(subgenre),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind idSubgenre.\n");
	fflush(stdout);
	return 0;
  }

  char *idSubgenre;
  rc = sqlite3_step(getIdStmt);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }else if(rc == SQLITE_ROW){
	char* take = (char*)sqlite3_column_text(getIdStmt,0);
	idSubgenre = (char*)malloc(strlen(take)+1);
	strcpy(idSubgenre,take);
  }
  sqlite3_finalize(getIdStmt);
  sqlite3_close(db3);

  if(strlen(idSubgenre) == 0){
	printf("[SQLITE]: There is no such subgenre.\n");
	fflush(stdout);
	return 0;
  }  

  rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }

  //insert song
  const char *query2 = "insert into songs (id_song,artist_name,song_name,votes) values (?,?,?,0)";
  sqlite3_stmt *statement2;

  //prepare query
  rc = sqlite3_prepare_v2(db, query2, -1, &statement2, NULL);
  if (rc != SQLITE_OK) {	
	printf("[SQLITE ERROR]: Couldn't prepare the query. %s\n",sqlite3_errmsg(db));
	fflush(stdout);
	sqlite3_close(db);
	return 0;
  }

  //bind first value
  rc = sqlite3_bind_int(statement2,1,index); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind index.\n");
	fflush(stdout);
	return 0;
  }

  //bind sencond value	
  rc = sqlite3_bind_text(statement2,2,artistName,strlen(artistName),SQLITE_TRANSIENT);
  if (rc != SQLITE_OK){
	printf("%s\n",sqlite3_errmsg(db));
	fflush(stdout);
	sqlite3_close(db);
	printf( "[SQL ERROR]: Couldn't bind artistName.\n");
	fflush(stdout);
	return 0;
  }

  rc = sqlite3_bind_text(statement2,3,songName,strlen(songName),SQLITE_TRANSIENT);
  if (rc != SQLITE_OK){
	printf("%s\n",sqlite3_errmsg(db));
	fflush(stdout);
	sqlite3_close(db);
	printf( "[SQL ERROR]: Couldn't bind songName.\n");
	fflush(stdout);
	return 0;
  }

  //execute statement
  rc = sqlite3_step(statement2);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }
  sqlite3_finalize(statement2);
  sqlite3_close(db);
  printf( "[SQLITE]: Song added.\n");
  fflush(stdout);

  sqlite3 *db2;
  char *db_name2 = "linksongs.sqlite";  

  rc = sqlite3_open(db_name2, &db2);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }

  //insert link song - subgenre
  const char* insertLink = "insert into linksongs (id_subgenre,id_song) values (?,?)";
  sqlite3_stmt *insertStmt;

  rc = sqlite3_prepare_v2(db2,insertLink,-1,&insertStmt,NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return 0;
  }

  //bind first value
  rc = sqlite3_bind_text(insertStmt,1,idSubgenre,strlen(idSubgenre),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind idSubgenre.\n");
	fflush(stdout);
	return 0;
  }

  //bind second value
  rc = sqlite3_bind_int(insertStmt,2,index); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind song_name.\n");
	fflush(stdout);
	return 0;
  }

  rc = sqlite3_step(insertStmt);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }
  return 1;
}

int addVote(char* artist,char* song){
  char *db_name = "songs.sqlite";
  sqlite3 *db;
  int rc;

  for(int i = 0;i < strlen(artist);i++)
	artist[i] = tolower(artist[i]);
  for(int i = 0;i < strlen(song);i++)
	song[i] = tolower(song[i]);

  rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }
	
  const char* query = "select votes from songs where lower(artist_name) like ? and lower(song_name) like ?";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db,query,-1,&statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return 0;
  }

  //bind first value
  rc = sqlite3_bind_text(statement,1,artist,strlen(artist),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind artist_name.\n");
	fflush(stdout);
	return 0;
  }

  //bind second value
  rc = sqlite3_bind_text(statement,2,song,strlen(song),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind song_name.\n");
	fflush(stdout);
	return 0;
  }

  int votes;
  //execute statement
  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }else{
	votes = (int)sqlite3_column_int(statement,0) + 1;
  }
  sqlite3_finalize(statement);
  const char* query2 = "update songs set votes = ? where lower(artist_name) = ? and lower(song_name) = ?";
  sqlite3_stmt* statement2;

  rc = sqlite3_prepare_v2(db,query2,-1,&statement2, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return 0;
  }

  //bind first value
  rc = sqlite3_bind_int(statement2,1,votes);
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind artist_name.\n");
	fflush(stdout);
	return 0;
  }

  //bind second value
  rc = sqlite3_bind_text(statement2,2,artist,strlen(artist),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind song_name.\n");
	fflush(stdout);
	return 0;
  }

  rc = sqlite3_bind_text(statement2,3,song,strlen(song),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind song_name.\n");
	fflush(stdout);
	return 0;
  }

  //execute statement
  rc = sqlite3_step(statement2);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }

  sqlite3_close(db);
  printf("[SQLITE]: Vote added.\n");
  fflush(stdout);
 return 1;
}

char* TopMusic50(){
  char* db_name = "songs.sqlite";
  char *artist,*song;
  char result[2000] = "";
  char *no = "NO";
  sqlite3 *db;

  //open database
  int rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open the songs.sqlite\n");
	fflush(stdout);
	return no;
  }
  const char* query = "select artist_name,song_name from songs order by votes,artist_name,song_name limit 50";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db,query,-1,&statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
        printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return no;
  }

  //execute statement
  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
        printf("[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return no;
  }else if(rc == SQLITE_ROW) {
	artist = (char*)sqlite3_column_text(statement,0);
	song = (char*)sqlite3_column_text(statement,1);
	strcpy(result,artist);
	strcat(result," - ");
	strcat(result,song);
  }

  while(sqlite3_step(statement) == SQLITE_ROW){
	artist = (char*)sqlite3_column_text(statement,0);
	song = (char*)sqlite3_column_text(statement,1);
	strcat(result,"\n");
 	strcat(result,artist);
	strcat(result," - ");
	strcat(result,song);
  }

  //printf("%s",result); for some reason returns null.
  sqlite3_close(db);
  //return result;
  char *msg;
  msg = (char*)malloc(strlen("[Top Music]: I've tried my best ¯\_(ツ)_/¯.") + 1);
  strcpy(msg,"[Top Music]: I've tried my best ¯\_(ツ)_/¯.");
  return msg;
}

char* TopMusicGenre(char *genre){
  char *msg;
  msg = (char*)malloc(strlen("[Top Music]: I've tried my best ¯\_(ツ)_/¯.") + 1);
  strcpy(msg,"[Top Music]: I've tried my best ¯\_(ツ)_/¯.");
  return msg;
}

char* addComment(char* artistName,char* songName){
  char *msg;
  msg = (char*)malloc(strlen("[Top Music]: I've tried my best ¯\_(ツ)_/¯.") + 1);
  strcpy(msg,"[Top Music]: I've tried my best ¯\_(ツ)_/¯.");
  return msg;
}

char *getComments(char *artist,char* song){
  char *msg;
  msg = (char*)malloc(strlen("[Top Music]: I've tried my best ¯\_(ツ)_/¯.") + 1);
  strcpy(msg,"[Top Music]: I've tried my best ¯\_(ツ)_/¯.");
  return msg;
}

const char* checkLogin(char* username,char* password){
  char* db_name = "users.sqlite";
  sqlite3 *db;

  //open database
  int rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	return "[SQLITE ERROR]: Couldn't open the users.sqlite";
  }
  const char *query = "select username from users where username like ? and password like ?";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db, query, -1, &statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
        return "[SQLITE ERROR]: Couldn't prepare the query.";
  }

  //bind first value
  rc = sqlite3_bind_text(statement,1,username,strlen(username),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	return "[SQL ERROR]: Couldn't bind username.";
  }

  //bind sencond value
  rc = sqlite3_bind_text(statement,2,password,strlen(password),SQLITE_TRANSIENT);
  if (rc != SQLITE_OK){
	printf("%s\n",sqlite3_errmsg(db));
	fflush(stdout);
	sqlite3_close(db);
	return "[SQL ERROR]: Couldn't bind password.";
  }

  int count = 0 ;
  //execute statement
  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
        return "[SQLITE ERROR]: Couldn't execute statement.";
  }else if(rc == SQLITE_ROW) {count++;}
 
  //return message
  if(count == 0) {
	sqlite3_close(db);
	return "[SQLITE]: No account found.";
  }
  sqlite3_close(db);
  return "[SQLITE]: The account exists.";
}

const char* addUser(char* username,char* password){
  char* db_name = "users.sqlite";
  sqlite3 *db;
  char* user;

  //open database
  int rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	return "[SQLITE ERROR]: Couldn't open the users.sqlite";
  }
  const char* checkUser = "select username from users where username like ?";
  sqlite3_stmt *checkStmt;

  //prepare query
  rc = sqlite3_prepare_v2(db,checkUser, -1, &checkStmt, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
        return "[SQLITE ERROR]: Couldn't prepare the query.";
  }

  //bind first value
  rc = sqlite3_bind_text(checkStmt,1,username,strlen(username),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	return "[SQL ERROR]: Couldn't bind username.";
  }
  
  int count = 0;
  //execute statement
  rc = sqlite3_step(checkStmt);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
        return "[SQLITE ERROR]: Couldn't execute statement.";
  }else if(rc == SQLITE_ROW) {
	user = (char*)sqlite3_column_text(checkStmt,0);
  } 
  //return message
  if(strcmp(username,user) == 0) {
	sqlite3_close(db);
	return "[SQLITE]: Username already exists.";
  }else{
	  const char *query = "insert into users (username,password,user_type,vote_perm,comm_perm) values (?,?,'basic',1,1)";
	  sqlite3_stmt *statement;

	  //prepare query
	  rc = sqlite3_prepare_v2(db, query, -1, &statement, NULL);
	  if (rc != SQLITE_OK) {
		sqlite3_close(db);
		return "[SQLITE ERROR]: Couldn't prepare the query.";
	  }

	  //bind first value
	  rc = sqlite3_bind_text(statement,1,username,strlen(username),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
	  if(rc != SQLITE_OK){
		sqlite3_close(db);
		return "[SQL ERROR]: Couldn't bind username(2).";
	  }

	  //bind sencond value
	  rc = sqlite3_bind_text(statement,2,password,strlen(password),SQLITE_TRANSIENT);
	  if (rc != SQLITE_OK){
		printf("%s\n",sqlite3_errmsg(db));
		fflush(stdout);
		sqlite3_close(db);
		return "[SQL ERROR]: Couldn't bind password.";
	  }

	  //execute statement
	  rc = sqlite3_step(statement);
	  if (rc == SQLITE_ERROR) {
		sqlite3_close(db);
		return "[SQLITE ERROR]: Couldn't execute statement.";
	  }

	  sqlite3_close(db);
	  return "[SQLITE]: Account added.";
  }
}

int deleteSong(char* artist,char* song){
  char* db_name = "songs.sqlite";
  sqlite3 *db;
  int id;

  for(int i = 0;i < strlen(artist);i++)
	artist[i] = tolower(artist[i]);
  for(int i = 0;i<strlen(song);i++)
	song[i] = tolower(song[i]);

  //open database
  int rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }
 
  const char* getId = "select id_song from songs where lower(artist_name) like ? and lower(song_name) like ?";
  sqlite3_stmt *idStmt;

  rc = sqlite3_prepare_v2(db,getId,-1,&idStmt, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
        printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return 0;
  }

  //bind first value
  rc = sqlite3_bind_text(idStmt,1,artist,strlen(artist),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind artist_name.\n");
	fflush(stdout);
	return 0;
  }

  rc = sqlite3_bind_text(idStmt,2,song,strlen(song),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind song_name.\n");
	fflush(stdout);
	return 0;
  }

  rc = sqlite3_step(idStmt);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
        printf("[SQLITE ERROR]: Couldn't execute statement.");
	fflush(stdout);
	return 0;
  }else if(rc == SQLITE_ROW) {
	id = sqlite3_column_int(idStmt,0);
  }

  const char* query = "delete from songs where id_song like ?";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db,query,-1,&statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
        printf("[SQLITE ERROR]: Couldn't prepare the query.");
	fflush(stdout);
	return 0;
  }

  //bind first value
  rc = sqlite3_bind_int(statement,1,id); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind id_song.");
	fflush(stdout);
	return 0;
  }

  //execute statement
  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
        printf("[SQLITE ERROR]: Couldn't execute statement.");
	fflush(stdout);
	return 0;
  }

  sqlite3_close(db);
  printf("[SQLITE]: Song deleted.");
  fflush(stdout);

  char* dbName = "linksongs.sqlite";
  sqlite3 *db2;

  //open database
  rc = sqlite3_open(dbName, &db2);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open the users.sqlite");
	fflush(stdout);
	return 0;
  }
  const char* query2 = "delete from linksongs where id_song like ?";
  sqlite3_stmt *statement2;

  //prepare query
  rc = sqlite3_prepare_v2(db2,query2,-1,&statement2, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db2);
        printf("[SQLITE ERROR]: Couldn't prepare the query.");
	fflush(stdout);
	return 0;
  }

  //bind first value
  rc = sqlite3_bind_int(statement2,1,id); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db2);
	printf("[SQL ERROR]: Couldn't bind id_song.\n");
	fflush(stdout);
	return 0;
  }

  //execute statement
  rc = sqlite3_step(statement2);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db2);
        printf("[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }

  sqlite3_close(db2);
  printf("[SQLITE]: Link deleted.\n");
  fflush(stdout);
  return 1;
}

int makeAdmin(char* username){
  char* db_name = "users.sqlite";
  sqlite3 *db;

  //open database
  int rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf( "[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }
  const char *query = "update users set user_type = 'admin' where username like ?";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db, query, -1, &statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
        printf( "[SQLITE ERROR]: Couldn't prepare the query.\n");
	return 0;
  }

  rc = sqlite3_bind_text(statement,1,username,strlen(username),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind username.\n");
	fflush(stdout);
	return 0;
  }

  //execute statement
  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
        printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }

  sqlite3_close(db);
  printf("[SQLITE]: Now admin.\n");
  fflush(stdout);
  return 1;
}

int unmakeAdmin(char* username){
  char* db_name = "users.sqlite";
  sqlite3 *db;

  //open database
  int rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf( "[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }
  const char *query = "update users set user_type = 'basic' where username like ?";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db, query, -1, &statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
        printf( "[SQLITE ERROR]: Couldn't prepare the query.\n");
	return 0;
  }

  rc = sqlite3_bind_text(statement,1,username,strlen(username),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind username.\n");
	fflush(stdout);
	return 0;
  }

  //execute statement
  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
        printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }

  sqlite3_close(db);
  printf("[SQLITE]: Now basic user.\n");
  fflush(stdout);
  return 1;
}

int blockPrivileges(char* username){
  char* db_name = "users.sqlite";
  sqlite3 *db;

  //open database
  int rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf( "[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }
  const char *query = "update users set vote_perm = 0, comm_perm = 0 where username like ?";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db, query, -1, &statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
        printf( "[SQLITE ERROR]: Couldn't prepare the query.\n");
	return 0;
  }

  rc = sqlite3_bind_text(statement,1,username,strlen(username),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind username.\n");
	fflush(stdout);
	return 0;
  }

  //execute statement
  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
        printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }

  sqlite3_close(db);
  printf("[SQLITE]: Blocked privileges.\n");
  fflush(stdout);
  return 1;
}

int unblockPrivileges(char* username){
  char* db_name = "users.sqlite";
  sqlite3 *db;

  //open database
  int rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf( "[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }
  const char *query = "update users set vote_perm = 1, comm_perm = 1 where username like ?";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db, query, -1, &statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
        printf( "[SQLITE ERROR]: Couldn't prepare the query.\n");
	return 0;
  }

  rc = sqlite3_bind_text(statement,1,username,strlen(username),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind username.\n");
	fflush(stdout);
	return 0;
  }

  //execute statement
  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
        printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }

  sqlite3_close(db);
  printf("[SQLITE]: Unblocked privileges.\n");
  fflush(stdout);
  return 1;
}

int checkAdmin(char* username){
  char* db_name = "users.sqlite";
  sqlite3 *db;

  //open database
  int rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf( "[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }
  const char *query = "select username from users where user_type like 'admin' and username like ?";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db, query, -1, &statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
        printf( "[SQLITE ERROR]: Couldn't prepare the query.\n");
	return 0;
  }

  rc = sqlite3_bind_text(statement,1,username,strlen(username),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind username.\n");
	fflush(stdout);
	return 0;
  }

  int count = 0 ;
  //execute statement
  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
        printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }else if(rc == SQLITE_ROW) {count++;}
 
  //printf( message)
  if(count == 0) {
	sqlite3_close(db);
	printf("[SQLITE]: Not and admin.\n");
	fflush(stdout);
	return 0;
  }else{
	  sqlite3_close(db);
	  printf("[SQLITE]: Admin.\n");
	  fflush(stdout);
	  return 1;
  }
}

char* songDetails(char* artist,char* song){
  char *db_name = "songs.sqlite";
  sqlite3 *db;
  int rc;
  char* no = "No";
  char* msg;
  char* link;
  char* description;

  rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return no;
  }
	
  const char* query = "select description from songs where artist_name like ? and song_name like ?";
  sqlite3_stmt *statement;

  //prepare query
  rc = sqlite3_prepare_v2(db,query,-1,&statement, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return no;
  }

  //bind first value
  rc = sqlite3_bind_text(statement,1,artist,strlen(artist),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind artist_name.\n");
	fflush(stdout);
	return no;
  }

  //bind second value
  rc = sqlite3_bind_text(statement,2,song,strlen(song),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind song_name.\n");
	fflush(stdout);
	return no;
  }

  //execute statement
  rc = sqlite3_step(statement);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return no;
  }else if (rc == SQLITE_ROW){
  	description = (char*)sqlite3_column_text(statement,0);
  }
  sqlite3_finalize(statement);

  const char* query2 = "select link from songs where artist_name like ? and song_name like ?";
  sqlite3_stmt *statement2;

  //prepare query
  rc = sqlite3_prepare_v2(db,query2,-1,&statement2, NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return no;
  }

  //bind first value
  rc = sqlite3_bind_text(statement2,1,artist,strlen(artist),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind artist_name.\n");
	fflush(stdout);
	return no;
  }

  //bind second value
  rc = sqlite3_bind_text(statement2,2,song,strlen(song),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind song_name.\n");
	fflush(stdout);
	return no;
  }

  //execute statement
  rc = sqlite3_step(statement2);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return no;
  }else if (rc == SQLITE_ROW){
  	link = (char*)sqlite3_column_text(statement2,0);
  }

  if(description && link){
	  msg = (char*)malloc(strlen(description) + strlen(link) + 9);
	  strcpy(msg,description);
	  strcat(msg,"\n\nLink: ");
	  strcat(msg,link);
  }else if(description && !link){
	  msg = (char*)malloc(strlen(description));
	  strcpy(msg,description);
  }else if (!description && link){
	  msg = (char*)malloc(strlen(link) + 8);
	  strcpy(msg,"Link: ");
	  strcat(msg,link);
  }else{
	msg = (char*)malloc(12);
	strcpy(msg,"No details.");
  }

  sqlite3_close(db);
  return msg;
}

int search(char* artist,char* song){
  char *db_name = "songs.sqlite";
  sqlite3 *db;
  int rc;

  for(int i=0;i < strlen(artist);i++)
	artist[i] = tolower(artist[i]);
  for(int i=0;i < strlen(song);i++)
	song[i] = tolower(song[i]);

  rc = sqlite3_open(db_name, &db);
  if(rc != SQLITE_OK){
	printf("[SQLITE ERROR]: Couldn't open the users.sqlite\n");
	fflush(stdout);
	return 0;
  }

  const char* searchQuery = "select artist_name,song_name from songs where lower(artist_name) like ? and lower(song_name) like ?";
  sqlite3_stmt *searchStmt;

  rc = sqlite3_prepare_v2(db,searchQuery,-1,&searchStmt,NULL);
  if (rc != SQLITE_OK) {
	sqlite3_close(db);
	printf("[SQLITE ERROR]: Couldn't prepare the query.\n");
	fflush(stdout);
	return 0;
  }

  //bind first value
  rc = sqlite3_bind_text(searchStmt,1,artist,strlen(artist),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind artist_name.\n");
	fflush(stdout);
	return 0;
  }

  //bind second value
  rc = sqlite3_bind_text(searchStmt,2,song,strlen(song),SQLITE_TRANSIENT); //SQLITE_TRANSIENT copy the string
  if(rc != SQLITE_OK){
	sqlite3_close(db);
	printf("[SQL ERROR]: Couldn't bind song_name.\n");
	fflush(stdout);
	return 0;
  }

  rc = sqlite3_step(searchStmt);
  if (rc == SQLITE_ERROR) {
	sqlite3_close(db);
	printf( "[SQLITE ERROR]: Couldn't execute statement.\n");
	fflush(stdout);
	return 0;
  }else if (rc == SQLITE_ROW){
	sqlite3_close(db);	
	return 1;
  }

  sqlite3_close(db);
  return 0;
}

int checkCmd(char* cmd){
    if(strcmp(cmd,"log_in") == 0) return 1;
    if(strcmp(cmd,"register") == 0) return 1;
    if(strcmp(cmd,"quit") == 0) return 1;
    return 0;
}

int checkCmdLogged(char* cmd,char* username){
    if(strcmp(cmd,"add_song") == 0) return 1;
    if(strcmp(cmd,"song_details") == 0) return 1;
    if(strcmp(cmd,"search") == 0) return 1;
    if(strcmp(cmd,"vote_song") == 0 && getPrivilege(username) == 1) return 1;
    if(strcmp(cmd,"top_songs") == 0) return 1;
    if(strcmp(cmd,"top_songs_genre") == 0) return 1;
    if(strcmp(cmd,"add_comment") == 0 && getPrivilege(username) == 1) return 1;
    if(strcmp(cmd,"comments") == 0 ) return 1;
    if(strcmp(cmd,"quit") == 0) return 1;
    if(checkAdmin(username)){
	    if(strcmp(cmd,"delete_song") == 0) return 1;
	    if(strcmp(cmd,"make_admin") == 0) return 1;
	    if(strcmp(cmd,"unmake_admin") == 0) return 1;
	    if(strcmp(cmd,"take_privileges") == 0) return 1;
	    if(strcmp(cmd,"give_privileges") == 0) return 1;
   }
   return 0;
}

int main(){
    struct sockaddr_in server;	// structura folosita de server
    struct sockaddr_in from;
    char cmd[50];		//mesajul primit de la client
    char msgrasp[3000]=" ";      //mesaj de raspuns pentru client
    int sd;			//descriptorul de socket
    int optval = 1;		//optiune folosita pentru setsockopt()
    int loggedIn = 0;

    printf("Preparing Top Music Server...\n");
    /* crearea unui socket */
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
      {
	perror ("[ERROR]: socket().\n");
        return errno;
      }

    /*setam pentru socket optiunea SO_REUSEADDR */
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    /* pregatirea structurilor de date */
    bzero (&server, sizeof (server));
    bzero (&from, sizeof (from));

    /* umplem structura folosita de server */
    /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;
    /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    /* utilizam un port utilizator */
    server.sin_port = htons (PORT);

    /* atasam socketul */
    if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
      {
        perror ("[ERROR]: bind().\n");
     	return errno;
      }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen (sd, 5) == -1)
      {
        perror ("[ERROR]: listen().\n");
      return errno;
      }

    printf("Listening at port 11111...\n");
    signal(SIGCHLD, SIG_IGN);

    /* servim in mod concurent clientii... */
    while (1)
      {
        int client;
        int length = sizeof (from);
        int pid;                        // id-ul procesului copil
        char address[INET_ADDRSTRLEN];  // adresa IP a clientului

        bzero(&from, sizeof(from));
        /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
        client = accept (sd, (struct sockaddr *) &from,(socklen_t*) &length);

        /* eroare la acceptarea conexiunii de la un client */
        if (client < 0)
          {
            perror ("[ERROR]: accept().\n");
            continue;
          }
        else
          {
            inet_ntop(AF_INET, &from.sin_addr.s_addr, address, INET_ADDRSTRLEN);
            printf("[Top Music Server-%d]: S-a conectat clientul: %s:%d\n", getpid(), address, ntohs(from.sin_port));
            fflush(stdout);
          }

        if ( (pid = fork()) < 0 ) {
          perror("[ERROR]: fork().");
	  continue;
        }
        else if (pid == 0) {    // procesul fiu care se va ocupa de client
	   char username[20];
	   char password[20];
	   bzero(username,20);
	   bzero(password,20);
           /* s-a realizat conexiunea, se trimite mesajul */
           bzero (msgrasp,2000);
           printf ("[Top Music Server-%d]: Sending intro message...\n", getpid());
           fflush (stdout);
	   strcpy(msgrasp,"[Top Music]: Welcome to Top Music! If you want to access the application you have to log in into your account. If you don't have an account you can easily register. You can use the commands: log_in / register / quit.\n");
	   if(write (client,msgrasp,2000) <= 0){
		printf("[ERROR][Top Music Server-%d]: write() to client.\n",getpid());
		close(client);
		continue;
	   }
	  
	   while(loggedIn == 0)
	   {
		/* citirea mesajului */
		if (read (client,cmd,50) <= 0)
		{
		    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
		    close (client);	/* inchidem conexiunea cu clientul */
		    continue;		/* continuam sa ascultam */
		}
		printf ("[Top Music Server-%d]: Got message %s.\n", getpid(), cmd);
		fflush(stdout);
		bzero(msgrasp,2000);

		//check if it's a command known
		if(checkCmd(cmd)){
		    //log in command
		    if(strcmp(cmd,"log_in") == 0){
			strcpy(msgrasp,"[Top Music]: Introduce your username:\n");
			if (write (client, msgrasp,2000) <= 0)
		    	{
			   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
			   continue;
		        }
			bzero(msgrasp,2000);
		        bzero(cmd,50);
			//read username
			if (read(client,cmd,50) <= 0){
			   printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
		   	   close (client);
		   	   continue;
			}
			printf("[Top Music]: Got username: %s\n",cmd);
			fflush(stdout);
			strcpy(username,cmd);
			strcpy(msgrasp,"[Top Music]: Introduce your password:\n");
			if (write (client, msgrasp,2000) <= 0)
		    	{
			   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
			   continue;		/* continuam sa ascultam */
		        }
			bzero(msgrasp,2000);
		        bzero(cmd,50);
			//read password
			if (read(client,cmd,50) <= 0){
			   printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
		   	   close (client);	/* inchidem conexiunea cu clientul */
		   	   continue;		/* continuam sa ascultam */
			}
			printf("[Top Music]: Got password: %s\n",cmd);
			fflush(stdout);			
			strcpy(password,cmd);
			//check if account exists
			printf("[Top Music]: %s\n",checkLogin(username,password));
			fflush(stdout);
			if(strcmp(checkLogin(username,password),"[SQLITE]: The account exists.") == 0){
				strcpy(msgrasp,"[Top Music]: Welcome back, ");
				strcat(msgrasp,username);
				strcat(msgrasp,". You can continue with add_song, song_details, search, vote_song,top_songs,top_songs_genre, add_comment, comments, delete_song, make_admin, unmake_admin, take_privileges, give_privileges or quit.\n");
				loggedIn = 1;
			}
			if(strcmp(checkLogin(username,password),"[SQLITE]: No account found.") == 0)
				strcpy(msgrasp,"[Top Music]: There is no such account. You can use the commands: log_in / register / quit.\n");
	        	if(write (client,msgrasp,2000) <= 0)
		    		{
			 	   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
			 	   continue;		/* continuam sa ascultam */
		        	}
			bzero(msgrasp,2000);
			bzero(cmd,50);
		    }
		    if(strcmp(cmd,"register") == 0){
			strcpy(msgrasp,"[Top Music]: After registration you will be automatically logged in. Introduce a username:\n");
			if (write (client, msgrasp,2000) <= 0)
		    	{
			   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
			   continue;
		        }
			bzero(msgrasp,2000);
		        bzero(cmd,50);
			//read username
			if (read(client,cmd,50) <= 0){
			   printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
		   	   close (client);
		   	   continue;
			}
			printf("[Top Music]: Got username: %s\n",cmd);
			fflush(stdout);
			strcpy(username,cmd);
			strcpy(msgrasp,"[Top Music]: Introduce a password:\n");
			if (write (client, msgrasp,2000) <= 0)
		    	{
			   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
			   continue;		/* continuam sa ascultam */
		        }
			bzero(msgrasp,2000);
		        bzero(cmd,50);
			//read password
			if (read(client,cmd,50) <= 0){
			   printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
		   	   close (client);	/* inchidem conexiunea cu clientul */
		   	   continue;		/* continuam sa ascultam */
			}
			printf("[Top Music]: Got password: %s\n",cmd);
			fflush(stdout);			
			strcpy(password,cmd);
			//check if account exists
			printf("[Top Music]: %s\n",addUser(username,password));
			fflush(stdout);
			if(strcmp(addUser(username,password),"[SQLITE]: Account added.") == 0){
				strcpy(msgrasp,"[Top Music]: Welcome to Top Music, ");
				strcat(msgrasp,username);
				strcat(msgrasp,". You can continue with add_song, song_details, search, vote_song,top_songs,top_songs_genre, add_comment, comments, delete_song, make_admin, unmake_admin, take_privileges, give_privileges or quit.\n");
				loggedIn = 1;
			}
			if(strcmp(addUser(username,password),"[SQLITE]: Username already exists.") == 0)
				strcpy(msgrasp,"[Top Music]: The username is taken. You can use the commands: log_in / register / quit.\n");
	        	if(write (client,msgrasp,2000) <= 0)
		    		{
			 	   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
			 	   continue;		/* continuam sa ascultam */
		        	}
			bzero(msgrasp,2000);
			bzero(cmd,50);
		    }
		    if(strcmp(cmd,"quit") == 0){
			exit(0);
		    }
		} //if command is a known command
		else
		{
		    strcpy(msgrasp,"[Top Music]: Not a known command.\n");
		    if (write (client,msgrasp,2000) <= 0)
		    {
			printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
			continue;		/* continuam sa ascultam */
		    }
		    bzero(msgrasp,2000);
		}
	   }//while not logged in
		
	   bzero(cmd,50);
	   while(loggedIn){
		if (read (client,cmd,50) <= 0)
		{
		    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
		    fflush(stdout);
		    close (client);	/* inchidem conexiunea cu clientul */
		    continue;		/* continuam sa ascultam */
		}
		printf("[Top Music]: Got message %s.\n",cmd);
		fflush(stdout);
	
		if(checkCmdLogged(cmd,username)){
			if(strcmp(cmd,"add_song") == 0){
				char artist[50];
				char song[50];
				char subgenre[20];
				strcpy(msgrasp,"[Top Music]: Introduce artist name:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(artist,cmd);
				strcpy(msgrasp,"[Top Music]: Introduce song name:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(song,cmd);
				strcpy(msgrasp,"[Top Music]: Introduce subgenre:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(subgenre,cmd);
				bzero(cmd,50);
				if(addSong(artist,song,subgenre) == 0) strcpy(msgrasp,"[Top Music]: Be sure the song you added is not already in the database or that the subgenre is spelled correctly.\n");
				else strcpy(msgrasp,"[Top Music]: Your song has been added. Continue with other command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"song_details") == 0){
				char artist[50];
				char song[50];
				strcpy(msgrasp,"[Top Music]: Introduce artist name:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(artist,cmd);
				strcpy(msgrasp,"[Top Music]: Introduce song name:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(song,cmd);
				bzero(cmd,50);
				strcpy(msgrasp,songDetails(artist,song));
				strcat(msgrasp,"\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"top_songs") == 0){
				strcpy(msgrasp,TopMusic50());
				strcat(msgrasp," You can choose another command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);				
			}
			if(strcmp(cmd,"top_songs_genre") == 0){
				strcpy(msgrasp,TopMusic50());
				strcat(msgrasp," You can choose another command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"add_comment") == 0){
				strcpy(msgrasp,TopMusic50());
				strcat(msgrasp," You can choose another command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"comments") == 0){
				strcpy(msgrasp,TopMusic50());
				strcat(msgrasp," You can choose another command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"delete_song") == 0){
				char artist[50];
				char song[50];
				strcpy(msgrasp,"[Top Music]: Introduce artist name:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(artist,cmd);
				strcpy(msgrasp,"[Top Music]: Introduce song name:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(song,cmd);
				bzero(cmd,50);
				if(deleteSong(artist,song)) strcpy(msgrasp,"[Top Music]: You have successfully deleted the song. You can continue with another command.\n");
				else strcpy(msgrasp,"[Top Music]: Something went wrong, but you can continue with another command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"make_admin") == 0){
				char user[50];
				strcpy(msgrasp,"[Top Music]: Introduce username to change type:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				strcpy(user,cmd);
				bzero(cmd,50);
				if(makeAdmin(user)) strcpy(msgrasp,"[Top Music]: User has been successfully changed to type 'admin'. You can continue with another command.\n");
				else strcpy(msgrasp,"[Top Music]: Something went wrong, but you can continue with another command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"unmake_admin") == 0){
				char user[50];
				strcpy(msgrasp,"[Top Music]: Introduce username to change type:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				strcpy(user,cmd);
				bzero(cmd,50);
				if(unmakeAdmin(user)) strcpy(msgrasp,"[Top Music]: User has been successfully changed to type 'basic'. You can continue with another command.\n");
				else strcpy(msgrasp,"[Top Music]: Something went wrong, but you can continue with another command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"take_privileges") == 0){
				char user[50];
				strcpy(msgrasp,"[Top Music]: Introduce username to take privileges:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				strcpy(user,cmd);
				bzero(cmd,50);
				if(blockPrivileges(user) == 1) strcpy(msgrasp,"[Top Music]: You have successfully taken privileges. You can continue with another command.\n");
				else strcpy(msgrasp,"[Top Music]: Something went wrong, but you can continue with another command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}	
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"give_privileges") == 0){
				char user[50];
				strcpy(msgrasp,"[Top Music]: Introduce username to give privileges:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				strcpy(user,cmd);
				bzero(cmd,50);
				if(unblockPrivileges(user) == 1) strcpy(msgrasp,"[Top Music]: You have successfully given privileges. You can continue with another command.\n");
				else strcpy(msgrasp,"[Top Music]: Something went wrong, but you can continue with another command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}	
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"vote_song") == 0){
				char artist[50];
				char song[50];
				strcpy(msgrasp,"[Top Music]: Introduce artist name:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(artist,cmd);
				strcpy(msgrasp,"[Top Music]: Introduce song name:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(song,cmd);
				bzero(cmd,50);
				if(addVote(artist,song)) strcpy(msgrasp,"[Top Music]: You vote has been added. You can choose another command.\n");
				else strcpy(msgrasp,"[Top Music]: Something went wrong :(, but you can choose another command.\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(msgrasp,2000);
			}
			if(strcmp(cmd,"quit") == 0){
				exit(0);
			}
			if(strcmp(cmd,"search") == 0){
				char artist[50];
				char song[50];
				strcpy(msgrasp,"[Top Music]: Introduce artist name:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(artist,cmd);
				strcpy(msgrasp,"[Top Music]: Introduce song name:\n");
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(cmd,50);
				bzero(msgrasp,2000);
				if (read (client,cmd,50) <= 0)
				{
				    printf ("[ERROR][Top Music Server-%d]: read() from client.\n", getpid());
				    fflush(stdout);
				    close (client);	/* inchidem conexiunea cu clientul */
				    continue;		/* continuam sa ascultam */
				}
				printf("[Top Music]: Got message %s.\n",cmd);
				fflush(stdout);
				strcpy(song,cmd);
				bzero(cmd,50);
				if(search(artist,song)) strcpy(msgrasp,"[Top Music]: The song has been found. Give another command.\n");
				else {
					strcpy(msgrasp,"[Top Music]: There is no such song from ");
					strcat(msgrasp,artist);
					strcat(msgrasp,". Give another command.\n");
				}
				if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
				bzero(msgrasp,2000);
			}
		}else{
			strcpy(msgrasp,"[Top Music]: Unknow command or admin only. Try another command.\n");
			if (write (client, msgrasp,2000) <= 0)
			    	{
				   printf ("[ERROR][Top Music Server-%d]: write() to client.\n", getpid());
				   fflush(stdout);
				   continue;		/* continuam sa ascultam */
				}
			bzero(msgrasp,2000);
		} //if command
	   } //while(loggedIn)
           /* terminarea executiei */
           exit(0);
        }		/* fork() */

        // procesul fiu are de asemenea descriptorul care refera socket-ul clientului,
        // astfel in procesul tata il putem inchide
	printf("[Top Music Server-%d]: closing connection with client.\n", getpid());
        close(client);
      }		//while(1)		
 return 0;
}
