/*
   This file is part of darktable,
   copyright (c) 2012 Jeremy Rosen

   darktable is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   darktable is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with darktable.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "lua/tags.h"
#include "lua/image.h"
#include "lua/types.h"
#include "common/darktable.h"
#include "common/tags.h"
#include "common/debug.h"


static int tag_name(lua_State*L)
{
  dt_lua_tag_t tagid1;
  luaA_to(L,dt_lua_tag_t,&tagid1,-2);
  gchar * name = dt_tag_get_name(tagid1);
  lua_pushstring(L,name);
  free(name);
  return 1;
}

static int tag_eq(lua_State*L)
{
  dt_lua_tag_t tagid1;
  luaA_to(L,dt_lua_tag_t,&tagid1,-1);
  dt_lua_tag_t tagid2;
  luaA_to(L,dt_lua_tag_t,&tagid2,-2);
  lua_pushboolean(L,tagid1==tagid2);
  return 1;
}
static int tag_tostring(lua_State *L)
{
  dt_lua_tag_t tagid1;
  luaA_to(L,dt_lua_tag_t,&tagid1,-1);
  gchar * name = dt_tag_get_name(tagid1);
  lua_pushstring(L,name);
  free(name);
  return 1;
}

static int tag_length(lua_State *L) {
  dt_lua_tag_t tagid;
  luaA_to(L,dt_lua_tag_t,&tagid,-1);
  sqlite3_stmt *stmt;
  int rv, count=-1;

  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db),
                              "SELECT count() FROM tagged_images WHERE tagid=?1", -1, &stmt, NULL);
  DT_DEBUG_SQLITE3_BIND_INT(stmt, 1, tagid);
  rv = sqlite3_step(stmt);
  if( rv != SQLITE_ROW)
    return luaL_error(L,"unknown SQL error");
  count = sqlite3_column_int(stmt,0);
  lua_pushnumber(L,count);
  sqlite3_finalize(stmt);
  return 1;
}
static int tag_index(lua_State *L) 
{
  dt_lua_tag_t tagid;
  luaA_to(L,dt_lua_tag_t,&tagid,-2);
  int index = luaL_checkinteger(L,-1);
  sqlite3_stmt *stmt = NULL;
  char query[1024];
  sprintf(query,"select imgid from tagged_images order by imgid limit 1 offset %d",index -1);
  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db),query, -1, &stmt, NULL);
  if(sqlite3_step(stmt) == SQLITE_ROW)
  {
    int imgid = sqlite3_column_int(stmt, 0);
    luaA_push(L,dt_lua_image_t,&imgid);
  }
  else
  {
    sqlite3_finalize(stmt);
    luaL_error(L,"incorrect index in database");
  }
  return 1;
}


static int tag_lib_length(lua_State *L) {
  sqlite3_stmt *stmt;
  int rv, count=-1;

  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db),
                              "SELECT count() FROM tags", -1, &stmt, NULL);
  rv = sqlite3_step(stmt);
  if( rv != SQLITE_ROW)
    return luaL_error(L,"unknown SQL error");
  count = sqlite3_column_int(stmt,0);
  lua_pushnumber(L,count);
  sqlite3_finalize(stmt);
  return 1;
}
static int tag_lib_index(lua_State *L) 
{
  int index = luaL_checkinteger(L,-1);
  sqlite3_stmt *stmt = NULL;
  char query[1024];
  sprintf(query,"SELECT id from tags order by id limit 1 offset %d",index -1);
  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db),query, -1, &stmt, NULL);
  if(sqlite3_step(stmt) == SQLITE_ROW)
  {
    int tagid = sqlite3_column_int(stmt, 0);
    luaA_push(L,dt_lua_tag_t,&tagid);
  }
  else
  {
    sqlite3_finalize(stmt);
    luaL_error(L,"incorrect index in database");
  }
  return 1;
}

static int tag_lib_create(lua_State *L)
{
  const char * name = luaL_checkstring(L,1);
  dt_lua_tag_t tagid;
  if(!dt_tag_new(name,&tagid)) {
    return luaL_error(L,"error creating tag %s\n",name);
  }
  luaA_push(L,dt_lua_tag_t,&tagid);
  return 1;
}

static int tag_delete(lua_State *L)
{
  dt_lua_tag_t tagid;
  luaA_to(L,dt_lua_tag_t,&tagid,-1);
  dt_tag_remove(tagid,true);
  return 0;
}


int dt_lua_tag_attach(lua_State *L)
{
  dt_lua_image_t imgid = -1;
  dt_lua_tag_t tagid = 0;
  if(luaL_testudata(L,1,"dt_lua_image_t")) {
    luaA_to(L,dt_lua_image_t,&imgid,1);
    luaA_to(L,dt_lua_tag_t,&tagid,2);
  } else {
    luaA_to(L,dt_lua_tag_t,&tagid,1);
    luaA_to(L,dt_lua_image_t,&imgid,2);
  }
  dt_tag_attach(tagid,imgid);
  return 0;
}

int dt_lua_tag_detach(lua_State *L)
{
  dt_lua_image_t imgid;
  dt_lua_tag_t tagid;
  if(luaL_testudata(L,1,"dt_lua_image_t")) {
    luaA_to(L,dt_lua_image_t,&imgid,1);
    luaA_to(L,dt_lua_tag_t,&tagid,2);
  } else {
    luaA_to(L,dt_lua_tag_t,&tagid,1);
    luaA_to(L,dt_lua_image_t,&imgid,2);
  }
  dt_tag_detach(tagid,imgid);
  return 0;
}

static int tag_lib_find(lua_State *L)
{
  const char * name = luaL_checkstring(L,1);
  dt_lua_tag_t tagid;
  if(!dt_tag_exists(name,&tagid)) {
    lua_pushnil(L);
    return 1;
  }
  luaA_push(L,dt_lua_tag_t,&tagid);
  return 1;
}

int dt_lua_tag_get_attached(lua_State *L)
{
  dt_lua_image_t imgid;
  luaA_to(L,dt_lua_image_t,&imgid,1);
  sqlite3_stmt *stmt;

  DT_DEBUG_SQLITE3_PREPARE_V2(dt_database_get(darktable.db),
                              "SELECT tagid FROM tagged_images WHERE imgid=?1", -1, &stmt, NULL);
  DT_DEBUG_SQLITE3_BIND_INT(stmt, 1, imgid);
  int rv = sqlite3_step(stmt);
  lua_newtable(L);
  while(rv == SQLITE_ROW) {
    int tagid = sqlite3_column_int(stmt, 0);
    luaA_push(L,dt_lua_tag_t,&tagid);
    luaL_ref(L,-2);
    rv = sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);
  return 1;
}


int dt_lua_init_tags(lua_State*L)
{
  dt_lua_init_type(L,dt_lua_tag_t);
  dt_lua_register_type_callback_number(L,dt_lua_tag_t,tag_index,NULL,tag_length);
  dt_lua_register_type_callback(L,dt_lua_tag_t,tag_name,NULL,"name",NULL);
  lua_pushcfunction(L,tag_delete);
  dt_lua_register_type_callback_stack(L,dt_lua_tag_t,"delete");
  lua_pushcfunction(L,dt_lua_tag_attach);
  dt_lua_register_type_callback_stack(L,dt_lua_tag_t,"attach");
  lua_pushcfunction(L,dt_lua_tag_detach);
  dt_lua_register_type_callback_stack(L,dt_lua_tag_t,"detach");
  luaL_getmetatable(L,"dt_lua_tag_t");
  lua_pushcfunction(L,tag_eq);
  lua_setfield(L,-2,"__eq");
  lua_pushcfunction(L,tag_tostring);
  lua_setfield(L,-2,"__tostring");
  lua_pop(L,1);

  dt_lua_push_darktable_lib(L);
  /* tags */
  typedef void* dt_lua_tags_lib;
  void* tmp = NULL;
  dt_lua_init_type(L,dt_lua_tags_lib);
  dt_lua_register_type_callback_number(L,dt_lua_tags_lib,tag_lib_index,NULL,tag_lib_length);
  lua_pushcfunction(L,tag_lib_create);
  dt_lua_register_type_callback_stack(L,dt_lua_tags_lib,"create");
  lua_pushcfunction(L,tag_lib_find);
  dt_lua_register_type_callback_stack(L,dt_lua_tags_lib,"find");
  lua_pushcfunction(L,tag_delete);
  dt_lua_register_type_callback_stack(L,dt_lua_tags_lib,"delete");
  lua_pushcfunction(L,dt_lua_tag_attach);
  dt_lua_register_type_callback_stack(L,dt_lua_tags_lib,"attach");
  lua_pushcfunction(L,dt_lua_tag_detach);
  dt_lua_register_type_callback_stack(L,dt_lua_tags_lib,"detach");
  lua_pushcfunction(L,dt_lua_tag_get_attached);
  dt_lua_register_type_callback_stack(L,dt_lua_tags_lib,"get_tags");
  luaA_push(L,dt_lua_tags_lib,&tmp);
  lua_setfield(L,-2,"tags");

  lua_pop(L,1);

  return 0;
}
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-space on;