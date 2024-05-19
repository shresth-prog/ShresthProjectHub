
@string(concat('call audit.SP_0000_ETL_PROCESS_ADD_TARGET_EXC('
,'''',concat(pipeline().parameters.p_root_pipeline_run_id,':childpl:',pipeline().RunId),''','
,'''',toUpper(pipeline().parameters.p_target_db),''','
,'''',toUpper(pipeline().parameters.p_target_schema),''','
,'''',toUpper(pipeline().parameters.p_target_table_name),''','
,'''',variables('v_cumulative_insert_count'),''','
,'''',variables('v_cumulative_update_count'),''','
,'''0'','
,'''0'
,''')'
))


CREATE OR REPLACE PROCEDURE SPARK_OPS_DEV.AUDIT.SP_0000_ETL_PROCESS_ADD_TARGET_EXC("PROCESS_RUN_ID" VARCHAR(16777216), "DB_NAME" VARCHAR(16777216), "SCHEMA_NAME" VARCHAR(16777216), "TABLE_NAME" VARCHAR(16777216), "ROWS_INSERTED" VARCHAR(16777216), "ROWS_UPDATED" VARCHAR(16777216), "ROWS_DELETED" VARCHAR(16777216), "ROWS_REJECTED" VARCHAR(16777216))
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
      sqlText: ''CALL SPARK_OPS_DEV.AUDIT.SP_0000_ETL_PROCESS_ADD_TARGET(:1,:2,:3,:4,:5,:6,:7,:8)'',
      binds: [PROCESS_RUN_ID, DB_NAME, SCHEMA_NAME, TABLE_NAME, ROWS_INSERTED, ROWS_UPDATED, ROWS_DELETED, ROWS_REJECTED]
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
    var plist     = [PROCESS_RUN_ID, DB_NAME, SCHEMA_NAME, TABLE_NAME, ROWS_INSERTED, ROWS_UPDATED, ROWS_DELETED, ROWS_REJECTED].toString();
    throw ''"procedure_call: '' + sql_call + '', parameter_list: ('' + plist + ''), error_message: '' + return_json.error_message + ''"'';
  }
  return return_json;

';