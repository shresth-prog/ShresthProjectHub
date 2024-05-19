

@string(concat('call audit.SP_0000_GET_BUSINESS_DATES_EXC('
,'''',variables('v_target_start_time'),''','
,'''',if(not(empty(pipeline().parameters.p_override_end_time)),pipeline().parameters.p_override_end_time,variables('v_process_start_tms')),''','
,'''',toUpper(pipeline().parameters.p_target_db),''','
,'''',toUpper(pipeline().parameters.p_target_schema),''','
,'''',toUpper(pipeline().parameters.p_target_table_name),''')'
))

CREATE OR REPLACE PROCEDURE SPARK_OPS_DEV.AUDIT.SP_0000_GET_BUSINESS_DATES_EXC("START_TMS" VARCHAR(16777216), "END_TMS" VARCHAR(16777216), "DB_NAME" VARCHAR(16777216), "SCHEMA_NAME" VARCHAR(16777216), "TABLE_NAME" VARCHAR(16777216))
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
      sqlText: ''CALL SPARK_OPS_DEV.AUDIT.SP_0000_GET_BUSINESS_DATES(:1,:2,:3,:4,:5)'',
      binds: [START_TMS, END_TMS, DB_NAME, SCHEMA_NAME, TABLE_NAME]
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
  
  //remove session timeouts      
  snowflake.execute({sqlText: "ALTER SESSION UNSET STATEMENT_TIMEOUT_IN_SECONDS"});

  if (return_json.result == ''ERROR''){
    var sql_call  = stmt.getSqlText();
    var plist     = [START_TMS, END_TMS, DB_NAME, SCHEMA_NAME, TABLE_NAME].toString();
    throw ''"procedure_call: '' + sql_call + '', parameter_list: ('' + plist + ''), error_message: '' + return_json.error_message + ''"'';
  }
  return return_json;

';

@string(concat('call audit.SP_0000_ETL_PROCESS_SET_STATUS_EXC('
,'''',concat(pipeline().parameters.p_root_pipeline_run_id,':childpl:',pipeline().RunId),''','
,'''ADF'','
,'''',variables('v_load_type'),''','
,'''',concat(pipeline().parameters.p_root_pipeline_name,':childpl:',pipeline().Pipeline),''','
, '''FAILED'','
,'''',variables('v_process_start_tms'),''','
,'''',convertTimeZone(utcNow(), 'UTC', 'New Zealand Standard Time','yyyy-MM-dd HH:mm:ss'),''','
,'''',replace(activity('lookup_get_business_dates').error.Message,'''',''''''),''')'
))

@string(activity('filter_intervals').output.FilteredItemsCount)
