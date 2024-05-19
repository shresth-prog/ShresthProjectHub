
@string(concat('call audit.SP_0000_ETL_PROCESS_SET_STATUS_EXC('
,'''',concat(pipeline().parameters.p_root_pipeline_run_id,':childpl:',pipeline().RunId),''','
,'''ADF'','
,'''',variables('v_load_type'),''','
,'''',concat(pipeline().parameters.p_root_pipeline_name,':childpl:',pipeline().Pipeline),''','
,'''COMPLETE'','
,'''',variables('v_process_start_tms'),''','
,'''',convertTimeZone(utcNow(), 'UTC', 'New Zealand Standard Time','yyyy-MM-dd HH:mm:ss'),''','
,'''',if(empty(replace(variables('v_error_msg'),'''','''''')),'',replace(variables('v_error_msg'),'''',''''''))
,''')'
))

CREATE OR REPLACE PROCEDURE SPARK_OPS_DEV.AUDIT.SP_0000_ETL_PROCESS_SET_STATUS_EXC("PROCESS_RUN_ID" VARCHAR(16777216), "PROCESS_APP" VARCHAR(16777216), "PROCESS_TYPE" VARCHAR(16777216), "PROCESS_NAME" VARCHAR(16777216), "PROCESS_STATUS" VARCHAR(16777216), "START_TMS" VARCHAR(16777216), "END_TMS" VARCHAR(16777216), "ERROR_INFO" VARCHAR(16777216))
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
  var retry_limit_query = 20;
  var retry_attempt_query = 0;
  
  var retry_limit_timeout = 5;
  var retry_attempt_timeout = 0;

  var stmt = snowflake.createStatement({
      sqlText: ''CALL SPARK_OPS_DEV.AUDIT.SP_0000_ETL_PROCESS_SET_STATUS(:1,:2,:3,:4,:5,:6,:7,:8)'',
      binds: [PROCESS_RUN_ID, PROCESS_APP, PROCESS_TYPE, PROCESS_NAME, PROCESS_STATUS, START_TMS, END_TMS, ERROR_INFO]
  });
  
  //set session timeouts for SQL statements
  snowflake.execute({sqlText: "ALTER SESSION SET STATEMENT_TIMEOUT_IN_SECONDS = 120, LOCK_TIMEOUT = 120"});
  
  while (retry_attempt_query < retry_limit_query || retry_attempt_timeout < retry_limit_timeout) {
  
    try {
	  //call child procedure
	  var result = stmt.execute();
	  
	  //1 test locking error, 2 unknown error (un/comment)
      //throw new Error(''"hjdcbdcbui waiters for this lock exceeds the sakjbcskidb"''); //1
      //throw new Error(''"hjdcbdcbui not the error you are looking for sakjbcskidb"'');    //2
	  //throw new Error(''"hjdcbdcbui was aborted because waiting for this lock is limited to 120 seconds sakjbcskidb"'');    //3
	  
	  result.next();
  
	  //completed procedure call    
      return_json = result.getColumnValue(1);

      //check for insert locking returned from child procedure
      if (return_json.result == ''ERROR''){
          if (return_json.error_message.search(''waiters for this lock exceeds the '') > 0 )  {
              retry_attempt_query++;
              retry_attempt_timeout=retry_limit_timeout;
              throw new Error(''"'' + return_json.error_message + ''"'');
          }
          else if (return_json.error_message.search(''was aborted because waiting for this lock is limited to 120 seconds'') > 0 )  {
              retry_attempt_timeout++;
              retry_attempt_query=retry_limit_query;
              throw new Error(''"'' + return_json.error_message + ''"'');
          }
      }
      retry_attempt_query=retry_limit_query;
      retry_attempt_timeout=retry_limit_timeout;

      } catch (err) {
         if (err.message.search(''waiters for this lock exceeds the '') > 0 )  { // lock limit exceeded
              sleep(15000); // 15 second delay
              return_json = {"result" : "ERROR", "error_message": "lock limit exceeded: " + err};           
            }
         else if (err.message.search(''was aborted because waiting for this lock is limited to 120 seconds'') > 0 )  { // lock timeout exceeded
              sleep(15000); // 15 second delay
              return_json = {"result" : "ERROR", "error_message": "lock timeout exceeded: " + err};           
            }
         else { //another error reason - exit loop     
              throw err;
         }
      }
  }

  //remove session timeouts      
  snowflake.execute({sqlText: "ALTER SESSION UNSET STATEMENT_TIMEOUT_IN_SECONDS, LOCK_TIMEOUT"});

  if (return_json.result == ''ERROR''){
    var sql_call  = stmt.getSqlText();
    var plist     = [PROCESS_RUN_ID, PROCESS_APP, PROCESS_TYPE, PROCESS_NAME, PROCESS_STATUS, START_TMS, END_TMS, ERROR_INFO].toString();
    throw ''"procedure_call: '' + sql_call + '', parameter_list: ('' + plist + ''), error_message: '' + return_json.error_message + ''"'';
  }
  return return_json;

';