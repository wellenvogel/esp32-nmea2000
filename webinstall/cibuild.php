<?php
include("token.php");
include("functions.php");
include("config.php");
mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);
include("cibuild_connection.php");
if (! isset($CI_TOKEN)) die("no token");
const apiBase="https://circleci.com/api/v2/";
const webApp="https://app.circleci.com/";
const apiRepo="project/gh/#user#/#repo#";
const workflowName="build-workflow";
const jobName="pio-build";
const defaultBranch='master';
const defaultUser='wellenvogel';
const defaultRepo='esp32-nmea2000';
const TABLENAME="CIBUILDS";

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
    $vcs=$pstat['vcs'];
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
    $pstat['vcs']=$vcs;
    return $pstat;
}

function getArtifacts($job,$slug){
    $url=apiBase."/project/$slug/$job/artifacts";
    return getJson($url,getTokenHeaders(),true);
}

function insertPipeline($id,$param){
    global $database;
    if (! isset($database)) return false;
    try {
        $status='created';
        $stmt = $database->prepare("INSERT into " . TABLENAME .
            "(id,status,config,environment,buildflags) VALUES (?,?,?,?,?)");
        $stmt->bind_param("sssss",
        $id,
        $status,
        $param['config'],
        $param['environment'],
        $param['build_flags']);
        $stmt->execute();
        return true;
    } catch (Exception $e) {
        error_log("insert pipeline $id failed: $e");
        return false;
    }
}
function updatePipeline($id,$status,$tag=null){
    global $database;
    if (! isset($database)) return false;
    try{
        $stmt=null;
        if ($tag != null){
            $stmt=$database->prepare("UPDATE ".TABLENAME." SET status=?,tag=? where id=? and ( status <> ? or tag <> ?)");
            $stmt->bind_param("sssss",$status,$tag,$id,$status,$tag);
            $stmt->execute();
        }
        else{
            $stmt=$database->prepare("UPDATE ".TABLENAME." SET status=? where id=? AND status <> ?");
            $stmt->bind_param("sss",$status,$id,$status);
            $stmt->execute();
        }

    }catch (Exception $e){
        error_log("update pipeline $id failed: $e");
        return false;
    }
    return true;
}

function findPipeline($param)
{
    global $database;
    if (!isset($database))
        return false;
    try {
        $stmt = null;
        if (isset($param['tag'])) {
            $stmt = $database->prepare("SELECT * from " . TABLENAME .
                " where status='success' and environment=? and buildflags=? and tag=? order by timestamp desc");
            $stmt->bind_param("sss", $param['environment'], $param['buildflags'], $param['tag']);
        } else {
            $stmt = $database->prepare("SELECT id from " . TABLENAME .
                " where status='success' and environment=? and buildflags=? order by timestamp desc");
            $stmt->bind_param("ss", $param['environment'], $param['buildflags']);
        }
        $stmt->execute();
        $id=null;
        $stmt->bind_result($id);
        if ($stmt->fetch()){
            return $id;
        }
        return false;
    } catch (Exception $e) {
        error_log("find pipeline failed: $e");
        return false;
    }

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
                if (isset($pstat['vcs'])){
                    updatePipeline($par['pipeline'],$pstat['status'],$pstat['vcs']['revision']);
                }
                else{
                    updatePipeline($par['pipeline'],$pstat['status']);
                }
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
        if ($action == 'pipelineuuid'){
            addVars(
                $par,
                ['pipeline']
            );
            $url=apiBase."/pipeline/".$par['pipeline'];
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
            insertPipeline($rt['id'],$requestParam['parameters']);
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
        header('Content-Disposition: attachment; filename="'.$astat['items'][0]['path'].'"');
        proxy($dlurl);
        exit(0);
    }
    if (isset($_REQUEST['find'])){
        $par=array();
        addVars($par,['environment','buildflags']);
        if (isset($_REQUEST['tag'])) $par['tag']=$_REQUEST['tag'];
        $id=findPipeline($par);
        header("Content-Type: application/json");
        $rt=array('status'=>'OK');
        if ($id){
            $rt['pipeline']=$id;
        }
        echo(json_encode($rt));
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