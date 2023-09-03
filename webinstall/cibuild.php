<?php
include("token.php");
include("functions.php");
include("config.php");
if (! isset($CI_TOKEN)) die("no token");
const apiBase="https://circleci.com/api/v2/";
const apiRepo="project/gh/#user#/#repo#";
const workflowName="build-workflow";
const jobName="pio-build";


function getTokenHeaders(){
    global $CI_TOKEN;
    return array('Circle-Token'=>$CI_TOKEN);
}
function getPipeline($pipeline){
    $url=apiBase."/pipeline/$pipeline";
    $token=getTokenHeaders();
    return getJson($url,$token,true);
}
function getWorkflow($pipeline,$workflowName){
    $url=apiBase."/pipeline/$pipeline/workflow";
    $token=getTokenHeaders();
    $pstate=getJson($url,$token,true);
    if (! isset($pstate['items'])){
        throw new Exception("no workflows in pipeline");
    }
    foreach ($pstate['items'] as $workflow){
        if (isset($workflow['name']) && $workflow['name'] == $workflowName){
            if (!isset($workflow['id'])){
                throw new Exception("no workflow id found");
            }
            return $workflow;
        }
    }
    throw new Exception("workflow $workflowName not found");
}
function getJob($pipeline,$workflow,$jobName){
    $url=apiBase."/workflow/".$workflow."/job";
    $token=getTokenHeaders();
    $wstate=getJson($url,$token,true);
    if (! isset($wstate['items'])){
        throw new Exception("no jobs in workflow");
    }
    foreach ($wstate['items'] as $job){
        if (isset($job['name']) && $job['name'] == $jobName){
            if (! isset($job['id'])){
                throw new Exception("no job id found");
            }
            return $job;
        }
    }
    throw new Exception("job $jobName not found");
}
function getJobStatus($pipeline,$wf=workflowName,$job=jobName){
    $pstat=getPipeline($pipeline);
    if (isset($pstat['error'])){
        throw new Exception($pstat["error"]);
    }
    if (! isset($pstat['state'])){
        throw new Exception("state not set");
    }
    if ($pstat['state'] != 'created'){
        return $pstat;
    }
    $pstat=getWorkflow($pipeline,$wf);
    $pstat=getJob($pipeline,$pstat['id'],$job);
    return $pstat;
}

function getArtifacts($job,$slug){
    $url=apiBase."/project/$slug/$job/artifacts";
    return getJson($url,getTokenHeaders(),true);
}

if (isset($_REQUEST['api'])){
    $action=$_REQUEST['api'];
    header("Content-Type: application/json");
    $par=array();
    if ($action == 'status') {
        addVars(
            $par,
            ['pipeline', 'workflow', 'job'],
            array('workflow' => workflowName, 'job' => jobName)
        );
        try {
            $pstat = getJobStatus($par['pipeline'], $par['workflow'], $par['job']);
            echo(json_encode($pstat));
        } catch (Exception $e) {
            $rt=array('status'=>'error','error'=>$e->getMessage());
            echo(json_encode($rt));
        }
        exit(0);
    }
    if ($action == 'artifacts'){
        addVars(
            $par,
            ['pipeline', 'workflow', 'job'],
            array('workflow' => workflowName, 'job' => jobName)
        );
        try{
            $jstat=getJobStatus($par['pipeline'], $par['workflow'], $par['job']);
            if (! isset($jstat['project_slug'])){
                throw new Exception("no project_slug in job");
            }
            if (! isset($jstat['status'])){
                throw new Exception("no job status");
            }
            if ($jstat['status'] != 'success'){
                throw new Exception("invalid job status ".$jstat['status']);
            }
            $astat=getArtifacts($jstat['job_number'],$jstat['project_slug']);
            echo (json_encode($astat));
        }catch (Exception $e){
            echo(json_encode(array('status'=>'error','error'=>$e->getMessage())));
        }
        exit(0);
    }
    die("invalid api $action");
}
die("no action");
?>