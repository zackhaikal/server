/*
   Copyright (c) 2015  MariaDB Foundation.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include "sql_type.h"
#include "sql_const.h"
#include "sql_class.h"
#include "item.h"
#include "log.h"


static Type_handler_tiny        type_handler_tiny;
static Type_handler_short       type_handler_short;
static Type_handler_long        type_handler_long;
static Type_handler_longlong    type_handler_longlong;
static Type_handler_int24       type_handler_int24;
static Type_handler_year        type_handler_year;
static Type_handler_bit         type_handler_bit;
static Type_handler_float       type_handler_float;
static Type_handler_double      type_handler_double;
static Type_handler_time        type_handler_time;
static Type_handler_time2       type_handler_time2;
static Type_handler_date        type_handler_date;
static Type_handler_newdate     type_handler_newdate;
static Type_handler_datetime    type_handler_datetime;
static Type_handler_datetime2   type_handler_datetime2;
static Type_handler_timestamp   type_handler_timestamp;
static Type_handler_timestamp2  type_handler_timestamp2;
static Type_handler_olddecimal  type_handler_olddecimal;
static Type_handler_newdecimal  type_handler_newdecimal;
static Type_handler_null        type_handler_null;
static Type_handler_string      type_handler_string;
static Type_handler_varchar     type_handler_varchar;
static Type_handler_tiny_blob   type_handler_tiny_blob;
static Type_handler_medium_blob type_handler_medium_blob;
static Type_handler_long_blob   type_handler_long_blob;
static Type_handler_blob        type_handler_blob;
#ifdef HAVE_SPATIAL
static Type_handler_geometry    type_handler_geometry;
#endif
static Type_handler_enum        type_handler_enum;
static Type_handler_set         type_handler_set;


/**
  This method is used by:
  - Item_user_var_as_out_param::field_type()
  - Item_func_udf_str::field_type()
  - Item_empty_string::make_field()

  TODO: type_handler_adjusted_to_max_octet_length() and string_type_handler()
  provide very similar functionality, to properly choose between
  VARCHAR/VARBINARY vs TEXT/BLOB variations taking into accoung maximum
  possible octet length.

  We should probably get rid of either of them and use the same method
  all around the code.
*/
const Type_handler *
Type_handler::string_type_handler(uint max_octet_length) const
{
  if (max_octet_length >= 16777216)
    return &type_handler_long_blob;
  else if (max_octet_length >= 65536)
    return &type_handler_medium_blob;
  return &type_handler_varchar;
}


/**
  This method is used by:
  - Item_sum_hybrid, e.g. MAX(item), MIN(item).
  - Item_func_set_user_var
*/
const Type_handler *
Type_handler_string_result::type_handler_adjusted_to_max_octet_length(
                                                        uint max_octet_length,
                                                        CHARSET_INFO *cs) const
{
  if (max_octet_length / cs->mbmaxlen <= CONVERT_IF_BIGGER_TO_BLOB)
    return &type_handler_varchar; // See also Item::too_big_for_varchar()
  if (max_octet_length >= 16777216)
    return &type_handler_long_blob;
  else if (max_octet_length >= 65536)
    return &type_handler_medium_blob;
  return &type_handler_blob;
}


const Type_handler *
Type_handler_hybrid_field_type::get_handler_by_result_type(Item_result type)
                                                           const
{
  switch (type) {
  case REAL_RESULT:       return &type_handler_double;
  case INT_RESULT:        return &type_handler_longlong;
  case DECIMAL_RESULT:    return &type_handler_newdecimal;
  case STRING_RESULT:     return &type_handler_long_blob;
  case TIME_RESULT:
  case ROW_RESULT:
    DBUG_ASSERT(0);
  }
  return &type_handler_string;
}


Type_handler_hybrid_field_type::Type_handler_hybrid_field_type()
  :m_type_handler(&type_handler_double)
{
}


const Type_handler *
Type_handler::get_handler_by_field_type(enum_field_types type)
{
  switch (type) {
  case MYSQL_TYPE_DECIMAL:     return &type_handler_olddecimal;
  case MYSQL_TYPE_NEWDECIMAL:  return &type_handler_newdecimal;
  case MYSQL_TYPE_TINY:        return &type_handler_tiny;
  case MYSQL_TYPE_SHORT:       return &type_handler_short;
  case MYSQL_TYPE_LONG:        return &type_handler_long;
  case MYSQL_TYPE_LONGLONG:    return &type_handler_longlong;
  case MYSQL_TYPE_INT24:       return &type_handler_int24;
  case MYSQL_TYPE_YEAR:        return &type_handler_year;
  case MYSQL_TYPE_BIT:         return &type_handler_bit;
  case MYSQL_TYPE_FLOAT:       return &type_handler_float;
  case MYSQL_TYPE_DOUBLE:      return &type_handler_double;
  case MYSQL_TYPE_NULL:        return &type_handler_null;
  case MYSQL_TYPE_VARCHAR:     return &type_handler_varchar;
  case MYSQL_TYPE_TINY_BLOB:   return &type_handler_tiny_blob;
  case MYSQL_TYPE_MEDIUM_BLOB: return &type_handler_medium_blob;
  case MYSQL_TYPE_LONG_BLOB:   return &type_handler_long_blob;
  case MYSQL_TYPE_BLOB:        return &type_handler_blob;
  case MYSQL_TYPE_VAR_STRING:  return &type_handler_varchar; // Map to VARCHAR 
  case MYSQL_TYPE_STRING:      return &type_handler_string;
  case MYSQL_TYPE_ENUM:        return &type_handler_varchar; // Map to VARCHAR
  case MYSQL_TYPE_SET:         return &type_handler_varchar; // Map to VARCHAR
  case MYSQL_TYPE_GEOMETRY:
#ifdef HAVE_SPATIAL
    return &type_handler_geometry;
#else
    return NULL;
#endif
  case MYSQL_TYPE_TIMESTAMP:   return &type_handler_timestamp2;// Map to timestamp2
  case MYSQL_TYPE_TIMESTAMP2:  return &type_handler_timestamp2;
  case MYSQL_TYPE_DATE:        return &type_handler_newdate;   // Map to newdate
  case MYSQL_TYPE_TIME:        return &type_handler_time2;     // Map to time2
  case MYSQL_TYPE_TIME2:       return &type_handler_time2;
  case MYSQL_TYPE_DATETIME:    return &type_handler_datetime2; // Map to datetime2
  case MYSQL_TYPE_DATETIME2:   return &type_handler_datetime2;
  case MYSQL_TYPE_NEWDATE:
    /*
      NEWDATE is actually a real_type(), not a field_type(),
      but it's used around the code in field_type() context.
      We should probably clean up the code not to use MYSQL_TYPE_NEWDATE
      in field_type() context and add DBUG_ASSERT(0) here.
    */
    return &type_handler_newdate;
  };
  DBUG_ASSERT(0);
  return &type_handler_string;
}


const Type_handler *
Type_handler::get_handler_by_real_type(enum_field_types type)
{
  /*
    VAR_STRING is actually a field_type(), not a real_type(),
    but it's used around the code in real_type() context.
    We should clean up the code and add DBUG_ASSERT(0) here.
  */
  if (type == MYSQL_TYPE_VAR_STRING)
    return &type_handler_string;
  return Type_handlers.handler(type);
}


/**
  Create a DOUBLE field by default.
*/
Field *
Type_handler::make_num_distinct_aggregator_field(MEM_ROOT *mem_root,
                                                 const Item *item) const
{
  return new(mem_root)
         Field_double(NULL, item->max_length,
                      (uchar *) (item->maybe_null ? "" : 0),
                      item->maybe_null ? 1 : 0, Field::NONE,
                      item->name, item->decimals, 0, item->unsigned_flag);
}


Field *
Type_handler_float::make_num_distinct_aggregator_field(MEM_ROOT *mem_root,
                                                       const Item *item)
                                                       const
{
  return new(mem_root)
         Field_float(NULL, item->max_length,
                     (uchar *) (item->maybe_null ? "" : 0),
                     item->maybe_null ? 1 : 0, Field::NONE,
                     item->name, item->decimals, 0, item->unsigned_flag);
}


Field *
Type_handler_decimal_result::make_num_distinct_aggregator_field(
                                                            MEM_ROOT *mem_root,
                                                            const Item *item)
                                                            const
{
  DBUG_ASSERT(item->decimals <= DECIMAL_MAX_SCALE);
  return new (mem_root)
         Field_new_decimal(NULL, item->max_length,
                           (uchar *) (item->maybe_null ? "" : 0),
                           item->maybe_null ? 1 : 0, Field::NONE,
                           item->name, item->decimals, 0, item->unsigned_flag);
}


Field *
Type_handler_int_result::make_num_distinct_aggregator_field(MEM_ROOT *mem_root,
                                                            const Item *item)
                                                            const
{
  /**
    Make a longlong field for all INT-alike types. It could create
    smaller fields for TINYINT, SMALLINT, MEDIUMINT, INT though.
  */
  return new(mem_root)
         Field_longlong(NULL, item->max_length,
                        (uchar *) (item->maybe_null ? "" : 0),
                        item->maybe_null ? 1 : 0, Field::NONE,
                        item->name, 0, item->unsigned_flag);
}


/***********************************************************************/

#define TMPNAME ""

Field *Type_handler_tiny::make_conversion_table_field(TABLE *table,
                                                      uint metadata,
                                                      const Field *target)
                                                      const
{
  /*
    As we don't know if the integer was signed or not on the master,
    assume we have same sign on master and slave.  This is true when not
    using conversions so it should be true also when using conversions.
  */
  bool unsigned_flag= ((Field_num*) target)->unsigned_flag;
  return new (table->in_use->mem_root)
         Field_tiny(NULL, 4 /*max_length*/, (uchar *) "", 1, Field::NONE,
                    TMPNAME, 0/*zerofill*/, unsigned_flag);
}


Field *Type_handler_short::make_conversion_table_field(TABLE *table,
                                                       uint metadata,
                                                       const Field *target)
                                                       const
{
  bool unsigned_flag= ((Field_num*) target)->unsigned_flag;
  return new (table->in_use->mem_root)
         Field_short(NULL, 6 /*max_length*/, (uchar *) "", 1, Field::NONE,
                     TMPNAME, 0/*zerofill*/, unsigned_flag);
}


Field *Type_handler_int24::make_conversion_table_field(TABLE *table,
                                                       uint metadata,
                                                       const Field *target)
                                                       const
{
  bool unsigned_flag= ((Field_num*) target)->unsigned_flag;
  return new (table->in_use->mem_root)
         Field_medium(NULL, 9 /*max_length*/, (uchar *) "", 1, Field::NONE,
                      TMPNAME, 0/*zerofill*/, unsigned_flag);
}


Field *Type_handler_long::make_conversion_table_field(TABLE *table,
                                                      uint metadata,
                                                      const Field *target)
                                                      const
{
  bool unsigned_flag= ((Field_num*) target)->unsigned_flag;
  return new (table->in_use->mem_root)
         Field_long(NULL, 11 /*max_length*/, (uchar *) "", 1, Field::NONE,
         TMPNAME, 0/*zerofill*/, unsigned_flag);
}


Field *Type_handler_longlong::make_conversion_table_field(TABLE *table,
                                                          uint metadata,
                                                          const Field *target)
                                                          const
{
  bool unsigned_flag= ((Field_num*) target)->unsigned_flag;
  return new (table->in_use->mem_root)
         Field_longlong(NULL, 20 /*max_length*/,(uchar *) "", 1, Field::NONE,
                        TMPNAME, 0/*zerofill*/, unsigned_flag);
}



Field *Type_handler_float::make_conversion_table_field(TABLE *table,
                                                       uint metadata,
                                                       const Field *target)
                                                       const
{
  return new (table->in_use->mem_root)
         Field_float(NULL, 12 /*max_length*/, (uchar *) "", 1, Field::NONE,
                     TMPNAME, 0/*dec*/, 0/*zerofill*/, 0/*unsigned_flag*/);
}


Field *Type_handler_double::make_conversion_table_field(TABLE *table,
                                                        uint metadata,
                                                        const Field *target)
                                                        const
{
  return new (table->in_use->mem_root)
         Field_double(NULL, 22 /*max_length*/, (uchar *) "", 1, Field::NONE,
                      TMPNAME, 0/*dec*/, 0/*zerofill*/, 0/*unsigned_flag*/);
}


Field *Type_handler_newdecimal::make_conversion_table_field(TABLE *table,
                                                            uint metadata,
                                                            const Field *target)
                                                            const
{
  int  precision= metadata >> 8;
  uint decimals= metadata & 0x00ff;
  uint32 max_length= my_decimal_precision_to_length(precision, decimals, false);
  DBUG_ASSERT(decimals <= DECIMAL_MAX_SCALE);
  return new (table->in_use->mem_root)
         Field_new_decimal(NULL, max_length, (uchar *) "", 1, Field::NONE,
                           TMPNAME, decimals, 0/*zerofill*/, 0/*unsigned*/);
}


Field *Type_handler_olddecimal::make_conversion_table_field(TABLE *table,
                                                            uint metadata,
                                                            const Field *target)
                                                            const
{
  sql_print_error("In RBR mode, Slave received incompatible DECIMAL field "
                  "(old-style decimal field) from Master while creating "
                  "conversion table. Please consider changing datatype on "
                  "Master to new style decimal by executing ALTER command for"
                  " column Name: %s.%s.%s.",
                  target->table->s->db.str,
                  target->table->s->table_name.str,
                  target->field_name);
  return NULL;
}


Field *Type_handler_year::make_conversion_table_field(TABLE *table,
                                                      uint metadata,
                                                      const Field *target)
                                                      const
{
  return new(table->in_use->mem_root)
         Field_year(NULL, 4, (uchar *) "", 1, Field::NONE, TMPNAME);
}


Field *Type_handler_null::make_conversion_table_field(TABLE *table,
                                                      uint metadata,
                                                      const Field *target)
                                                      const
{
  return new(table->in_use->mem_root)
         Field_null(NULL, 0, Field::NONE, TMPNAME, target->charset());
}


Field *Type_handler_timestamp::make_conversion_table_field(TABLE *table,
                                                           uint metadata,
                                                           const Field *target)
                                                           const
{
  // We assume TIMESTAMP(0)
  return new(table->in_use->mem_root)
         Field_timestamp(NULL, MAX_DATETIME_WIDTH, (uchar *) "", 1,
                         Field::NONE, TMPNAME, table->s);
}


Field *Type_handler_timestamp2::make_conversion_table_field(TABLE *table,
                                                            uint metadata,
                                                            const Field *target)
                                                            const
{
  return new(table->in_use->mem_root)
         Field_timestampf(NULL, (uchar *) "", 1, Field::NONE,
                          TMPNAME, table->s, metadata);
}


Field *Type_handler_newdate::make_conversion_table_field(TABLE *table,
                                                         uint metadata,
                                                         const Field *target)
                                                         const
{
  return new(table->in_use->mem_root)
         Field_newdate(NULL, (uchar *) "", 1, Field::NONE, TMPNAME);
}


Field *Type_handler_date::make_conversion_table_field(TABLE *table,
                                                      uint metadata,
                                                      const Field *target)
                                                      const
{
  return new(table->in_use->mem_root)
         Field_date(NULL, (uchar *) "", 1, Field::NONE, TMPNAME);
}


Field *Type_handler_time::make_conversion_table_field(TABLE *table,
                                                      uint metadata,
                                                      const Field *target)
                                                      const
{
  return new(table->in_use->mem_root)
         Field_time(NULL, MAX_TIME_WIDTH, (uchar *) "", 1,
                    Field::NONE, TMPNAME);
}


Field *Type_handler_time2::make_conversion_table_field(TABLE *table,
                                                       uint metadata,
                                                       const Field *target)
                                                       const
{
  return new(table->in_use->mem_root)
         Field_timef(NULL, (uchar *) "", 1, Field::NONE, TMPNAME, metadata);
}


Field *Type_handler_datetime::make_conversion_table_field(TABLE *table,
                                                          uint metadata,
                                                          const Field *target)
                                                          const
{
  return new(table->in_use->mem_root)
         Field_datetime(NULL, MAX_DATETIME_WIDTH, (uchar *) "", 1,
                        Field::NONE, TMPNAME);
}


Field *Type_handler_datetime2::make_conversion_table_field(TABLE *table,
                                                           uint metadata,
                                                           const Field *target)
                                                           const
{
  return new(table->in_use->mem_root)
         Field_datetimef(NULL, (uchar *) "", 1,
                         Field::NONE, TMPNAME, metadata);
}


Field *Type_handler_bit::make_conversion_table_field(TABLE *table,
                                                     uint metadata,
                                                     const Field *target)
                                                     const
{
  DBUG_ASSERT((metadata & 0xff) <= 7);
  uint32 max_length= 8 * (metadata >> 8U) + (metadata & 0x00ff);
  return new(table->in_use->mem_root)
         Field_bit_as_char(NULL, max_length, (uchar *) "", 1,
                           Field::NONE, TMPNAME);
}


Field *Type_handler_string::make_conversion_table_field(TABLE *table,
                                                        uint metadata,
                                                        const Field *target)
                                                        const
{
  /* This is taken from Field_string::unpack. */
  uint32 max_length= (((metadata >> 4) & 0x300) ^ 0x300) + (metadata & 0x00ff);
  return new(table->in_use->mem_root)
         Field_string(NULL, max_length, (uchar *) "", 1,
                      Field::NONE, TMPNAME, target->charset());
}


Field *Type_handler_varchar::make_conversion_table_field(TABLE *table,
                                                         uint metadata,
                                                         const Field *target)
                                                         const
{
  return new(table->in_use->mem_root)
         Field_varstring(NULL, metadata, HA_VARCHAR_PACKLENGTH(metadata),
                         (uchar *) "", 1, Field::NONE, TMPNAME,
                         table->s, target->charset());
}


Field *Type_handler_tiny_blob::make_conversion_table_field(TABLE *table,
                                                           uint metadata,
                                                           const Field *target)
                                                           const
{
  return new(table->in_use->mem_root)
         Field_blob(NULL, (uchar *) "", 1, Field::NONE, TMPNAME,
                    table->s, 1, target->charset());
}


Field *Type_handler_blob::make_conversion_table_field(TABLE *table,
                                                      uint metadata,
                                                      const Field *target)
                                                      const
{
  return new(table->in_use->mem_root)
         Field_blob(NULL, (uchar *) "", 1, Field::NONE, TMPNAME,
                    table->s, 2, target->charset());
}


Field *Type_handler_medium_blob::make_conversion_table_field(TABLE *table,
                                                           uint metadata,
                                                           const Field *target)
                                                           const
{
  return new(table->in_use->mem_root)
         Field_blob(NULL, (uchar *) "", 1, Field::NONE, TMPNAME,
                    table->s, 3, target->charset());
}


Field *Type_handler_long_blob::make_conversion_table_field(TABLE *table,
                                                           uint metadata,
                                                           const Field *target)
                                                           const
{
  return new(table->in_use->mem_root)
         Field_blob(NULL, (uchar *) "", 1, Field::NONE, TMPNAME,
                    table->s, 4, target->charset());
}


#ifdef HAVE_SPATIAL
Field *Type_handler_geometry::make_conversion_table_field(TABLE *table,
                                                          uint metadata,
                                                          const Field *target)
                                                          const
{
  DBUG_ASSERT(target->type() == MYSQL_TYPE_GEOMETRY);
  /*
    We do not do not update feature_gis statistics here:
    status_var_increment(target->table->in_use->status_var.feature_gis);
    as this is only a temporary field.
    The statistics was already incremented when "target" was created.
  */
  return new(table->in_use->mem_root)
         Field_geom(NULL, (uchar *) "", 1, Field::NONE, TMPNAME, table->s, 4,
                    ((const Field_geom*) target)->geom_type,
                    ((const Field_geom*) target)->srid);
}
#endif

Field *Type_handler_enum::make_conversion_table_field(TABLE *table,
                                                      uint metadata,
                                                      const Field *target)
                                                      const
{
  DBUG_ASSERT(target->type() == MYSQL_TYPE_STRING);
  DBUG_ASSERT(target->real_type() == MYSQL_TYPE_ENUM);
  return new(table->in_use->mem_root)
         Field_enum(NULL, target->field_length,
                    (uchar *) "", 1, Field::NONE, TMPNAME,
                    metadata & 0x00ff/*pack_length()*/,
                    ((const Field_enum*) target)->typelib, target->charset());
}


Field *Type_handler_set::make_conversion_table_field(TABLE *table,
                                                     uint metadata,
                                                     const Field *target)
                                                     const
{
  DBUG_ASSERT(target->type() == MYSQL_TYPE_STRING);
  DBUG_ASSERT(target->real_type() == MYSQL_TYPE_SET);
  return new(table->in_use->mem_root)
         Field_set(NULL, target->field_length,
                   (uchar *) "", 1, Field::NONE, TMPNAME,
                   metadata & 0x00ff/*pack_length()*/,
                   ((const Field_enum*) target)->typelib, target->charset());
}


/*************************************************************************/
Field *Type_handler_olddecimal::make_table_field(MEM_ROOT *mem_root,
                                                 TABLE_SHARE *share,
                                                 const char *field_name,
                                                 const Record_addr &rec,
                                                 const Create_attr &attr) const
{
  return new (mem_root)
    Field_decimal(rec.ptr, attr.length(), rec.null_ptr, rec.null_bit,
                  attr.unireg_check(), field_name,
                  f_decimals(attr.pack_flag()),
                  f_is_zerofill(attr.pack_flag()) != 0,
                  f_is_dec(attr.pack_flag()) == 0);
}


Field *Type_handler_newdecimal::make_table_field(MEM_ROOT *mem_root,
                                                 TABLE_SHARE *share,
                                                 const char *field_name,
                                                 const Record_addr &rec,
                                                 const Create_attr &attr) const
{
  return new (mem_root)
    Field_new_decimal(rec.ptr, attr.length(), rec.null_ptr, rec.null_bit,
                      attr.unireg_check(), field_name,
                      f_decimals(attr.pack_flag()),
                      f_is_zerofill(attr.pack_flag()) != 0,
                      f_is_dec(attr.pack_flag()) == 0);
}


Field *Type_handler_float::make_table_field(MEM_ROOT *mem_root,
                                            TABLE_SHARE *share,
                                            const char *field_name,
                                            const Record_addr &rec,
                                            const Create_attr &attr) const
{
  return new (mem_root)
    Field_float(rec.ptr, attr.length(), rec.null_ptr, rec.null_bit,
                attr.unireg_check(), field_name,
                f_decimals(attr.pack_flag()),
                f_is_zerofill(attr.pack_flag()) != 0,
                f_is_dec(attr.pack_flag())== 0);
}


Field *Type_handler_double::make_table_field(MEM_ROOT *mem_root,
                                             TABLE_SHARE *share,
                                             const char *field_name,
                                             const Record_addr &rec,
                                             const Create_attr &attr) const
{
  return new (mem_root)
    Field_double(rec.ptr, attr.length(), rec.null_ptr, rec.null_bit,
                 attr.unireg_check(), field_name,
                 f_decimals(attr.pack_flag()),
                 f_is_zerofill(attr.pack_flag()) != 0,
                 f_is_dec(attr.pack_flag())== 0);
}


Field *Type_handler_tiny::make_table_field(MEM_ROOT *mem_root,
                                           TABLE_SHARE *share,
                                           const char *field_name,
                                           const Record_addr &rec,
                                           const Create_attr &attr) const
{
  return new (mem_root)
    Field_tiny(rec.ptr, attr.length(), rec.null_ptr, rec.null_bit,
               attr.unireg_check(), field_name,
               f_is_zerofill(attr.pack_flag()) != 0,
               f_is_dec(attr.pack_flag()) == 0);
}


Field *Type_handler_short::make_table_field(MEM_ROOT *mem_root,
                                            TABLE_SHARE *share,
                                            const char *field_name,
                                            const Record_addr &rec,
                                            const Create_attr &attr) const
{
  return new (mem_root)
    Field_short(rec.ptr, attr.length(), rec.null_ptr, rec.null_bit,
                attr.unireg_check(), field_name,
                f_is_zerofill(attr.pack_flag()) != 0,
                f_is_dec(attr.pack_flag()) == 0);
}


Field *Type_handler_int24::make_table_field(MEM_ROOT *mem_root,
                                            TABLE_SHARE *share,
                                            const char *field_name,
                                            const Record_addr &rec,
                                            const Create_attr &attr) const
{
  return new (mem_root)
    Field_medium(rec.ptr, attr.length(), rec.null_ptr, rec.null_bit,
                 attr.unireg_check(), field_name,
                 f_is_zerofill(attr.pack_flag()) != 0,
                 f_is_dec(attr.pack_flag()) == 0);
}


Field *Type_handler_long::make_table_field(MEM_ROOT *mem_root,
                                           TABLE_SHARE *share,
                                           const char *field_name,
                                           const Record_addr &rec,
                                           const Create_attr &attr) const
{
  return new (mem_root)
    Field_long(rec.ptr, attr.length(), rec.null_ptr, rec.null_bit,
               attr.unireg_check(), field_name,
               f_is_zerofill(attr.pack_flag()) != 0,
               f_is_dec(attr.pack_flag()) == 0);
}


Field *Type_handler_longlong::make_table_field(MEM_ROOT *mem_root,
                                               TABLE_SHARE *share,
                                               const char *field_name,
                                               const Record_addr &rec,
                                               const Create_attr &attr) const
{
  return new (mem_root)
    Field_longlong(rec.ptr, attr.length(), rec.null_ptr, rec.null_bit,
                   attr.unireg_check(), field_name,
                   f_is_zerofill(attr.pack_flag()) != 0,
                   f_is_dec(attr.pack_flag()) == 0);
}



Field *Type_handler_timestamp::make_table_field(MEM_ROOT *mem_root,
                                                TABLE_SHARE *share,
                                                const char *field_name,
                                                const Record_addr &rec,
                                                const Create_attr &attr) const
{
  uint dec= attr.length() > MAX_DATETIME_WIDTH ?
            attr.length() - MAX_DATETIME_WIDTH - 1: 0;
  return new_Field_timestamp(mem_root, rec.ptr, rec.null_ptr, rec.null_bit,
                             attr.unireg_check(), field_name, share, dec);
}


Field *Type_handler_timestamp2::make_table_field(MEM_ROOT *mem_root,
                                                 TABLE_SHARE *share,
                                                 const char *field_name,
                                                 const Record_addr &rec,
                                                 const Create_attr &attr) const
{
  uint dec= attr.length() > MAX_DATETIME_WIDTH ?
            attr.length() - MAX_DATETIME_WIDTH - 1: 0;
  return new (mem_root)
    Field_timestampf(rec.ptr, rec.null_ptr, rec.null_bit,
                     attr.unireg_check(), field_name, share, dec);
}


Field *Type_handler_year::make_table_field(MEM_ROOT *mem_root,
                                           TABLE_SHARE *share,
                                           const char *field_name,
                                           const Record_addr &rec,
                                           const Create_attr &attr) const
{
  return new (mem_root)
    Field_year(rec.ptr, attr.length(), rec.null_ptr, rec.null_bit,
               attr.unireg_check(), field_name);

}


Field *Type_handler_date::make_table_field(MEM_ROOT *mem_root,
                                           TABLE_SHARE *share,
                                           const char *field_name,
                                           const Record_addr &rec,
                                           const Create_attr &attr) const
{
  return new (mem_root)
    Field_date(rec.ptr, rec.null_ptr, rec.null_bit,
               attr.unireg_check(), field_name);
}


Field *Type_handler_newdate::make_table_field(MEM_ROOT *mem_root,
                                              TABLE_SHARE *share,
                                              const char *field_name,
                                              const Record_addr &rec,
                                              const Create_attr &attr) const
{
  return new (mem_root)
    Field_newdate(rec.ptr, rec.null_ptr, rec.null_bit,
                  attr.unireg_check(), field_name);
}


Field *Type_handler_time::make_table_field(MEM_ROOT *mem_root,
                                           TABLE_SHARE *share,
                                           const char *field_name,
                                           const Record_addr &rec,
                                           const Create_attr &attr) const
{
  uint dec= attr.length() > MIN_TIME_WIDTH ?
            attr.length() - MIN_TIME_WIDTH - 1: 0;
  return new_Field_time(mem_root, rec.ptr, rec.null_ptr, rec.null_bit,
                        attr.unireg_check(), field_name, dec);
}




Field *Type_handler_time2::make_table_field(MEM_ROOT *mem_root,
                                            TABLE_SHARE *share,
                                            const char *field_name,
                                            const Record_addr &rec,
                                            const Create_attr &attr) const
{
  uint dec= attr.length() > MIN_TIME_WIDTH ?
            attr.length() - MIN_TIME_WIDTH - 1: 0;
  return new (mem_root)
    Field_timef(rec.ptr, rec.null_ptr, rec.null_bit,
                attr.unireg_check(), field_name, dec);
}


Field *Type_handler_datetime::make_table_field(MEM_ROOT *mem_root,
                                               TABLE_SHARE *share,
                                               const char *field_name,
                                               const Record_addr &rec,
                                               const Create_attr &attr) const
{
  uint dec= attr.length() > MAX_DATETIME_WIDTH ?
            attr.length() - MAX_DATETIME_WIDTH - 1: 0;
  return new_Field_datetime(mem_root, rec.ptr, rec.null_ptr, rec.null_bit,
                            attr.unireg_check(), field_name, dec);
}


Field *Type_handler_datetime2::make_table_field(MEM_ROOT *mem_root,
                                                TABLE_SHARE *share,
                                                const char *field_name,
                                                const Record_addr &rec,
                                                const Create_attr &attr) const
{
  uint dec= attr.length() > MAX_DATETIME_WIDTH ?
            attr.length() - MAX_DATETIME_WIDTH - 1: 0;
  return new (mem_root)
    Field_datetimef(rec.ptr, rec.null_ptr, rec.null_bit,
                    attr.unireg_check(), field_name, dec);
}


Field *Type_handler_null::make_table_field(MEM_ROOT *mem_root,
                                           TABLE_SHARE *share,
                                           const char *field_name,
                                           const Record_addr &rec,
                                           const Create_attr &attr) const
{
  return new (mem_root)
    Field_null(rec.ptr, attr.length(), attr.unireg_check(),
               field_name, attr.charset());
}


/*************************************************************************/

Type_handler_register::Type_handler_register()
  :m_min_type(256), m_max_type(0)
{
  add(&type_handler_tiny);
  add(&type_handler_short);
  add(&type_handler_long);
  add(&type_handler_int24);
  add(&type_handler_longlong);
  add(&type_handler_year);
  add(&type_handler_bit);
  add(&type_handler_float);
  add(&type_handler_double);

  add(&type_handler_time);
  add(&type_handler_time2);

  add(&type_handler_date);
  add(&type_handler_newdate);

  add(&type_handler_datetime);
  add(&type_handler_datetime2);

  add(&type_handler_timestamp);
  add(&type_handler_timestamp2);

  add(&type_handler_olddecimal);
  add(&type_handler_newdecimal);

  add(&type_handler_null);

  add(&type_handler_string);
  add(&type_handler_varchar);

  add(&type_handler_tiny_blob);
  add(&type_handler_medium_blob);
  add(&type_handler_long_blob);
  add(&type_handler_blob);

#ifdef HAVE_SPATIAL
  add(&type_handler_geometry);
#endif

  add(&type_handler_enum);
  add(&type_handler_set);
}

Type_handler_register Type_handlers;
