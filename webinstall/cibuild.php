<?php
include("token.php");
include("functions.php");
include("config.php");
if (! isset($CI_TOKEN)) die("no token");
const apiBase="https://circleci.com/api/v2/";
const webApp="https://app.circleci.com/";
const apiRepo="project/gh/#user#/#repo#";
const workflowName="build-workflow";
const jobName="pio-build";
const defaultBranch='circleci-project-setup';
const defaultUser='wellenvogel';
const defaultRepo='esp32-nmea2000';

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
    $pipeline_id=$pstat['id'];
    $pipeline_number=$pstat['number'];
    $pstat=getWorkflow($pipeline,$wf);
    $workflow_id=$pstat['id'];
    $workflow_number=$pstat['workflow_number'];
    $pstat=getJob($pipeline,$pstat['id'],$job);
    $pstat['pipeline_id']=$pipeline_id;
    $pstat['pipeline_number']=$pipeline_number;
    $pstat['workflow_id']=$workflow_id;
    $pstat['workflow_number']=$workflow_number;
    if (isset($pstat['project_slug'])){
        $pstat['status_url']=webApp."/pipelines/".
            preg_replace('/^gh/','github',$pstat['project_slug'])."/".
            $pipeline_number."/workflows/".$workflow_id."/jobs/".$pstat['job_number'];
    }
    return $pstat;
}

function getArtifacts($job,$slug){
    $url=apiBase."/project/$slug/$job/artifacts";
    return getJson($url,getTokenHeaders(),true);
}

function getArtifactsForPipeline($pipeline,$wf=workflowName,$job=jobName){
    $jstat=getJobStatus($pipeline,$wf,$job);
    if (! isset($jstat['job_number'])){
        throw new Exception("no job number");
    }
    if (! isset($jstat['status'])){
        throw new Exception("no job status");
    }
    if ($jstat['status'] != 'success'){
        throw new Exception("invalid job status ".$jstat['status']);
    }
    $astat=getArtifacts($jstat['job_number'],$jstat['project_slug']);
    return $astat;
}
try {
    if (isset($_REQUEST['api'])) {
        $action = $_REQUEST['api'];
        header("Content-Type: application/json");
        $par = array();
        if ($action == 'status') {
            addVars(
                $par,
                ['pipeline', 'workflow', 'job'],
                array('workflow' => workflowName, 'job' => jobName)
            );
            try {
                $pstat = getJobStatus($par['pipeline'], $par['workflow'], $par['job']);
                echo (json_encode($pstat));
            } catch (Exception $e) {
                $rt = array('status' => 'error', 'error' => $e->getMessage());
                echo (json_encode($rt));
            }
            exit(0);
        }
        if ($action == 'artifacts') {
            addVars(
                $par,
                ['pipeline', 'workflow', 'job'],
                array('workflow' => workflowName, 'job' => jobName)
            );
            try {
                $astat = getArtifactsForPipeline($par['pipeline'], $par['workflow'], $par['job']);
                echo (json_encode($astat));
            } catch (Exception $e) {
                echo (json_encode(array('status' => 'error', 'error' => $e->getMessage())));
            }
            exit(0);
        }
        if ($action == 'pipeline'){
            addVars(
                $par,
                ['number','user','repo'],
                array('user'=>defaultUser,'repo'=>defaultRepo)
            );
            $url=apiBase."/".replaceVars(apiRepo,fillUserAndRepo(null,$par))."/pipeline/".$par['number'];
            $rt=getJson($url,getTokenHeaders(),true);
            echo(json_encode($rt));
            exit(0);
        }
        if ($action == 'start'){
            addVars(
                $par,
                ['environment','buildflags','config','suffix','branch','user','repo'],
                array('suffix'=>'',
                    'branch'=>defaultBranch,
                    'config'=>'{}',
                    'user'=>defaultUser,
                    'repo'=>defaultRepo,
                    'buildflags'=>''
                    )
            );
            $requestParam=array(
                'branch'=>$par['branch'],
                'parameters'=> array(
                    'run_build'=>true,
                    'environment'=>$par['environment'],
                    'suffix'=>$par['suffix'],
                    'config'=>$par['config'],
                    'build_flags'=>$par['buildflags']
                )
            );
            $userRepo=fillUserAndRepo(null,$par);
            $url=apiBase."/".replaceVars(apiRepo,$userRepo)."/pipeline";
            $rt=getJson($url,getTokenHeaders(),true,$requestParam);
            echo (json_encode($rt));
            exit(0);
        }
        throw new Exception("invalid api $action");
    }
    if (isset($_REQUEST['download'])) {
        $pipeline = $_REQUEST['download'];
        $par = array('pipeline' => $pipeline);
        addVars(
            $par,
            ['workflow', 'job'],
            array('workflow' => workflowName, 'job' => jobName)
        );
        $astat = getArtifactsForPipeline($par['pipeline'], $par['workflow'], $par['job']);
        if (!isset($astat['items']) || count($astat['items']) < 1) {
            die("no artifacts for job");
        }
        $dlurl = $astat['items'][0]['url'];
        #echo("DL: $dlurl\n");
        proxy($dlurl);
        exit(0);
    }
    die("no action");
} catch (HTTPErrorException $h) {
    header($_SERVER['SERVER_PROTOCOL'] . " " . $h->code . " " . $h->getMessage());
    die($h->getMessage());
} catch (Exception $e) {
    header($_SERVER['SERVER_PROTOCOL'] . ' 500 ' . $e->getMessage());
    die($e->getMessage());
}
?>