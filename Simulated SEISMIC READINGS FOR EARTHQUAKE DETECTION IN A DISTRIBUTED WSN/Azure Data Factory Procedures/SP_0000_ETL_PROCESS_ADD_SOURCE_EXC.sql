@string(concat('call audit.SP_0000_ETL_PROCESS_ADD_SOURCE_EXC('
,'''',concat(pipeline().parameters.p_root_pipeline_run_id,':childpl:',pipeline().RunId),''','
,'''DATABASE'','
,'''SNOWFLAKE'','
,'''',if(empty(pipeline().parameters.p_source_schema),'NA',toUpper(pipeline().parameters.p_target_db)),''','
,'''',if(empty(pipeline().parameters.p_source_schema),'NA',toUpper(pipeline().parameters.p_source_schema)),''','
,'''',replace(replace(replace(replace(replace(replace(replace(replace(replace(pipeline().parameters.p_source_query
,'SPARK_DL.', concat(pipeline().globalParameters.snowflake_dl_db,'.'))
,'SPARK_DW.', concat(pipeline().globalParameters.snowflake_dw_db,'.'))
,'SPARK_ODS.', concat(pipeline().globalParameters.snowflake_ods_db,'.'))
,'SPARK_OPS.', concat(pipeline().globalParameters.snowflake_ops_db,'.'))
,'spark_dl.', concat(pipeline().globalParameters.snowflake_dl_db,'.'))
,'spark_dw.', concat(pipeline().globalParameters.snowflake_dw_db,'.'))
,'spark_ods.', concat(pipeline().globalParameters.snowflake_ods_db,'.'))
,'spark_ops.', concat(pipeline().globalParameters.snowflake_ops_db,'.')),'''',''''''),''','
,''''','''','''',''0'
,''')'
))



CREATE OR REPLACE PROCEDURE SPARK_OPS_DEV.AUDIT.SP_0000_ETL_PROCESS_ADD_SOURCE_EXC("PROCESS_RUN_ID" VARCHAR(16777216), "SOURCE_TYPE" VARCHAR(16777216), "SOURCE_NAME" VARCHAR(16777216), "DB_NAME" VARCHAR(16777216), "SCHEMA_NAME" VARCHAR(16777216), "TABLE_NAME" VARCHAR(16777216), "FILE_NAME" VARCHAR(16777216), "FILE_FORMAT" VARCHAR(16777216), "FILE_TYPE" VARCHAR(16777216), "ROW_COUNT" VARCHAR(16777216))
RETURNS VARIANT
LANGUAGE JAVASCRIPT
EXECUTE AS CALLER
AS '
  
  // ‘wait’ function
  function sleep(milliseconds) {
    const date = Date.now();
    let currentDate = null;
    do {
        currentDate = Date.now();
    } while (currentDate - date < milliseconds);
  }
  
  var return_json = {};
  var retry_limit = 20;
  var retry_attempt = 0;

  var stmt = snowflake.createStatement({
      sqlText: ''CALL SPARK_OPS_DEV.AUDIT.SP_0000_ETL_PROCESS_ADD_SOURCE(:1,:2,:3,:4,:5,:6,:7,:8,:9,:10)'',
      binds: [PROCESS_RUN_ID, SOURCE_TYPE, SOURCE_NAME, DB_NAME, SCHEMA_NAME, TABLE_NAME, FILE_NAME, FILE_FORMAT, FILE_TYPE, ROW_COUNT]
  });
  
  //set session timeout for SQL statements
  snowflake.execute({sqlText: "ALTER SESSION SET STATEMENT_TIMEOUT_IN_SECONDS = 120"});

  while (retry_attempt < retry_limit) {
  
    retry_attempt++;

    try {
	  var result = stmt.execute();
	  
	  //1 test locking error, 2 unknown error (un/comment)
      //throw new Error(''"hjdcbdcbui waiters for this lock exceeds the sakjbcskidb"''); //1
      //throw new Error(''"hjdcbdcbui not the error you are looking for sakjbcskidb"'');    //2
	  
	  result.next();
  
	  //completed procedure call    
      return_json = result.getColumnValue(1);

      //check for insert locking returned from child procedure
      if (return_json.result == ''ERROR''){
          if (return_json.error_message.search(''waiters for this lock exceeds the '') > 0 )  {
              throw new Error(''"'' + return_json.error_message + ''"'');
          }
      }
      retry_attempt=20;

      } catch (err) {
         if (err.message.search(''waiters for this lock exceeds the '') > 0 )  { // lock limit exceeded
              sleep(15000); // 15 second delay
              return_json = {"result" : "ERROR", "error_message": "lock limit exceeded: " + err};

            }
         else { //another error reason - exit loop     
              throw err;
         }
      }
  }
  
  //remove session timeout      
  snowflake.execute({sqlText: "ALTER SESSION UNSET STATEMENT_TIMEOUT_IN_SECONDS"});

  if (return_json.result == ''ERROR''){
    var sql_call  = stmt.getSqlText();
    var plist     = [PROCESS_RUN_ID, SOURCE_TYPE, SOURCE_NAME, DB_NAME, SCHEMA_NAME, TABLE_NAME, FILE_NAME, FILE_FORMAT, FILE_TYPE, ROW_COUNT].toString();
    throw ''"procedure_call: '' + sql_call + '', parameter_list: ('' + plist + ''), error_message: '' + return_json.error_message + ''"'';
  }
  return return_json;

';
