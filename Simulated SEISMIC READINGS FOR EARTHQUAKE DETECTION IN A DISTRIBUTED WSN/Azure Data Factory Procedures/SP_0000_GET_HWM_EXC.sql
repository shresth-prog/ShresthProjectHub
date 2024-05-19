@string(concat('call audit.SP_0000_GET_HWM_EXC('
,'''',toUpper(pipeline().parameters.p_target_db),''','
,'''',toUpper(pipeline().parameters.p_target_schema),''','
,'''',toUpper(pipeline().parameters.p_target_table_name),''','
,'''',pipeline().parameters.p_root_pipeline_name,''')'
))

CREATE OR REPLACE PROCEDURE SPARK_OPS_DEV.AUDIT.SP_0000_GET_HWM_EXC("DB_NAME" VARCHAR(16777216), "SCH_NAME" VARCHAR(16777216), "TABLE_NAME" VARCHAR(16777216), "PROCESS_NAME" VARCHAR(16777216))
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
      sqlText: ''CALL SPARK_OPS_DEV.AUDIT.SP_0000_GET_HWM(:1,:2,:3,:4)'',
      binds: [DB_NAME, SCH_NAME, TABLE_NAME, PROCESS_NAME]
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
    var plist     = [DB_NAME, SCH_NAME, TABLE_NAME, PROCESS_NAME].toString();
    throw ''"procedure_call: '' + sql_call + '', parameter_list: ('' + plist + ''), error_message: '' + return_json.error_message + ''"'';
  }
  return return_json;

';

@if(or(equals(pipeline().parameters.p_override_start_time,''),equals(pipeline().parameters.p_override_start_time,null)),
if(equals(json(activity('lookup_get_hwm').output.firstRow.SP_0000_GET_HWM_EXC).high_watermark_tms,'1970-01-01  00:00:00'),
variables('v_process_start_tms'),
json(activity('lookup_get_hwm').output.firstRow.SP_0000_GET_HWM_EXC).high_watermark_tms)
,pipeline().parameters.p_override_start_time)