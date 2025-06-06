
Write-Host "Number of clients to start:"
$n_client = Read-Host

if ($n_client -lt 1)
{
    Write-Host "Try again, enter a valid number of clients!"
    exit
}

Start-Process powershell -ArgumentList  "-NoExit", "-Command", "docker-compose up --build --scale client=$n_client"


$allContainersRunning = $false
while (-not $allContainersRunning)
{

    $containers = docker ps --format "{{.Names}}"

    $allContainersRunning = $true
    for ($i = 1; $i -le $n_client; $i++) 
    {
        $containerName = "tris-lso-client-$i"
        if (-not ($containers -contains $containerName)) 
        {
            $allContainersRunning = $false
            break
        }
    }
  
    if (-not $allContainersRunning)
    {
        Start-Sleep -Seconds 2
    }
}

for ($i = 1; $i -le $n_client; $i++)
{
    Start-Process powershell -ArgumentList "-NoExit", "-Command", "docker attach progetto-lso-client-$i"
}