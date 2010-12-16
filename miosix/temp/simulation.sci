clear
global SkedData
SkedData = struct('SP_Tr',0,'alfa',0,'alfaBlock',0,...
                  'krr',0,'zrr',0,'kpi',0,'bmin',0,'bmax',0,...
                  'SP_Tp',0,...
                  'eTp',0,'eTr',0,'eTro',0,'bc',0,'bco',0,'b',0,'bo',0,...
                  'Tp',0,'Tr',0,'t',0,...
                  'dmod',0,'dmodo',0);
global SimRes
SimRes   = struct('Ntot',0,...
                  'NfirstCur',0,....
                  'mt',[],...
                  'mSP_Tr',[],'mTr',[],...
                  'mSP_Tp',[],'mTp',[],...
                  'mb',[],'mbc',[]);
                  
function s=step(t)
         if t>=0 s=1; else s=0; end;
endfunction    

function r=ramp(t)
         r=t*step(t);
endfunction  

function ExecSkedRegulators()
         global SkedData;
         // External PI (round duration)
         SkedData.eTr   = SkedData.SP_Tr-SkedData.Tr;

         SkedData.bc    = SkedData.bco+SkedData.krr*SkedData.eTr-SkedData.krr*SkedData.zrr*SkedData.eTro;
         
         SkedData.bco   = SkedData.bc;
         
         SkedData.eTro  = SkedData.eTr;
         SkedData.SP_Tp = SkedData.alfa*(SkedData.bc+SkedData.Tr);
         // Internal I with AW (CPU usage within round)
         SkedData.eTp   = SkedData.SP_Tp-SkedData.Tp;
         SkedData.b     = SkedData.bo+SkedData.kpi*SkedData.eTp;     
         SkedData.b     = min(max(SkedData.b,SkedData.bmin),SkedData.bmax);
         SkedData.bo    = SkedData.b;
endfunction

function ExecProcPool()
         global SkedData;
         // Generate burst disturbances (use multiplicative)
         x              = max(1.1,1+0.8*(rand()-0.5));
         SkedData.dmod  = 1;//0.9*SkedData.dmodo+0.1*x;
         SkedData.dmodo = SkedData.dmod;
         // Compute processes and round times
         SkedData.Tp    = max(SkedData.b.*SkedData.dmod.*(ones(SkedData.b)-SkedData.alfaBlock),0);
         SkedData.Tr    = sum(SkedData.Tp);
         SkedData.t     = SkedData.t+SkedData.Tr;
endfunction

function AddProcessAtBottom()
         global SkedData;
         SkedData.alfa      = [SkedData.alfa;0];
         SkedData.alfaBlock = [SkedData.alfaBlock;0];
         SkedData.Tp        = [SkedData.Tp;0];
         SkedData.bo        = [SkedData.bo;0];
endfunction

function RemProcessFromTop()
         global SkedData;
         global SimRes;
         N                  = length(SkedData.alfa);
         SkedData.alfa      = [SkedData.alfa(2:N)];
         SkedData.alfaBlock = [SkedData.alfaBlock(2:N)];
         SkedData.Tp        = [SkedData.Tp(2:N)];
         SkedData.bo        = [SkedData.bo(2:N)];
         SimRes.NfirstCur   = SimRes.NfirstCur+1;
endfunction

function RemProcessFromBottom()
         global SkedData;
         global SimRes;
         N                  = length(SkedData.alfa);
         SkedData.alfa      = [SkedData.alfa(1:N-1)];
         SkedData.alfaBlock = [SkedData.alfaBlock(1:N-1)];
         SkedData.Tp        = [SkedData.Tp(1:N-1)];
         SkedData.bo        = [SkedData.bo(1:N-1)];
         SimRes.NfirstCur   = SimRes.NfirstCur;
endfunction

function GetSimRes()
        global SkedData;
        global SimRes;
        SimRes.mt     = [SimRes.mt,SkedData.t];
        SimRes.mSP_Tr = [SimRes.mSP_Tr,SkedData.SP_Tr];
        SimRes.mTr    = [SimRes.mTr,SkedData.Tr];
        SimRes.mbc    = [SimRes.mbc,SkedData.bc];
        N             = length(SkedData.alfa);
        dex           = SimRes.NfirstCur:SimRes.NfirstCur+N-1;
        cnan          = zeros(SimRes.Ntot,1);//%nan*ones(SimRes.Ntot,1);
        vSP_Tp        = cnan;
        vSP_Tp(dex)   = SkedData.alfa*(SkedData.Tr+SkedData.bc);
        SimRes.mSP_Tp = [SimRes.mSP_Tp,vSP_Tp];
        vTp           = cnan;
        vTp(dex)      = SkedData.Tp;
        SimRes.mTp    = [SimRes.mTp,vTp];
        vb            = cnan;
        vb(dex)       = SkedData.b;
        SimRes.mb     = [SimRes.mb,vb];
endfunction


//--------------------------------------------


SimRes.Ntot        = 12;             // Total number of processes entering/exiting the test
SimRes.NfirstCur   = 1;              // Index of the first process of running set
SkedData.alfa      = [0.2 0.3 0.5]'; // which consists of three processes
SkedData.alfaBlock = [0   0   0 ]';  // 1 blocks the process, 0 makes it run

// Controller parameters
SkedData.kpi     = 0.25;
SkedData.krr     = 1.4;
SkedData.zrr     = 0.88;
SkedData.bmin    = 0;
SkedData.bmax    = 1.5;

// Simulation duration
Nrounds          = 8000; // 8000


for i=1:Nrounds //SkedData.t
    SkedData.SP_Tr = 1+0.005*ramp(i-0200)-0.005*ramp(i-400)...
                      -0.010*ramp(i-1200)+0.010*ramp(i-1300)...
                      +0.002*ramp(i-2200)-0.002*ramp(i-2600)...
                      -0.004*ramp(i-3200)+0.004*ramp(i-3400)...
                      -0.800*step(i-3800)+1.300*step(i-4500)...
                      +0.025*ramp(i-5500)-0.025*ramp(i-5600)...
                      -0.025*ramp(i-6200)+0.025*ramp(i-6300);
                      
    //if i==0030
    //   SkedData.alfaBlock = [0   1   0 ]';
    //end;    
    //if i==0100
    //   SkedData.alfaBlock = [0   0   0 ]';
    //end;   
    if i==0500
       AddProcessAtBottom();
       SkedData.alfa    = [0.2 0.25 0.4 0.15]';
    end;   
    if i==1000
       SkedData.alfa    = [0.1 0.15 0.2 0.55]';
    end;
    if i==2000
       AddProcessAtBottom();
       AddProcessAtBottom();
       AddProcessAtBottom();
       AddProcessAtBottom();
       SkedData.alfa=[0.06 0.08 0.10 0.12 0.14 0.15 0.17 0.18]';
    end;
    //if i==2100
    //   SkedData.alfaBlock = [0 0 1 0 0 1 0 1]';
    //end;    
    //if i==2450
    //   SkedData.alfaBlock = [0 0 0 0 0 0 0 0]';
    //end;  
    if i==2500
       RemProcessFromBottom();
       RemProcessFromBottom();
       SkedData.alfa=[0.05 0.05 0.05 0.10 0.10 0.65]';
    end;
    if i==3000
       AddProcessAtBottom();    
       SkedData.alfa=[0.60 0.05 0.05 0.10 0.10 0.05 0.05]';
    end;
    if i==4000
       RemProcessFromBottom();
       RemProcessFromBottom();
       SkedData.alfa=[0.8 0.05 0.05 0.05 0.05]';
    end;
    if i==4500
       AddProcessAtBottom();
       SkedData.alfa=[0.1 0.1 0.2 0.2 0.2 0.2]';
    end;
    if i==4800
       RemProcessFromBottom();
       AddProcessAtBottom();
       AddProcessAtBottom();
       SkedData.alfa=[0.30 0.05 0.06 0.07 0.10 0.15 0.27]';
    end;
    ExecSkedRegulators();
    ExecProcPool();
    GetSimRes();
end

//SimRes.mt = 1:length(SimRes.mt);

hf=scf(0); clf;
hf.figure_size = [1100,700];
drawlater();
subplot(221); //SimRes.mt
   plot([1:1:8000],SimRes.mSP_Tr,'r');
   plot([1:1:8000],SimRes.mTr,'k');
   plot([1:1:8000],SimRes.mbc,'g');   
   title('Round duration (black) vs set point (red) and burst correction (green)');
   ax = get("current_axes");
   ax.data_bounds = [0,0;8000,4.5]; // [0,0;max(SimRes.mt),4.5]
   ax.tight_limits = "on";
   ylabel("time units");
subplot(222);
   plot([1:1:8000],SimRes.mSP_Tp,'r');
   plot([1:1:8000],SimRes.mTp,'k');
   title('Processes CPU use (black) vs set point (red)');
   ax = get("current_axes");
   ax.data_bounds = [0,0;8000,1.5]; // [0,0;max(SimRes.mt),1.5];
   ax.tight_limits = "on";
subplot(223);
   for i = 0:SimRes.Ntot-1
        //plot([0,max(SimRes.mt)],[1.5*i,1.5*i],'y');
        plot([1:1:8000],SimRes.mSP_Tp(i+1,:)+1.5*i,'r');
        plot([1:1:8000],SimRes.mTp(i+1,:)+1.5*i,'k');
        //plot([0,max(SimRes.mt)],[1.5*i,1.5*i]+1,'g');
   end
   title('Individual processes CPU use');
   ax = get("current_axes");
   ax.data_bounds = [0,0;8000,12]; // [0,0;max(SimRes.mt),18];
   ax.tight_limits = "on";
   xlabel("time units");
subplot(224);
   for i = 0:SimRes.Ntot-1
        plot([0,max(SimRes.mt)],[1.5*i,1.5*i],'y');
        plot([1:1:8000],(SimRes.mb(i+1,:)...
             -SkedData.bmin)/(SkedData.bmax-SkedData.bmin)+1.5*i,'k');
        plot([0,max(SimRes.mt)],[1.5*i,1.5*i]+1,'g');
   end
   title('Normalised bursts');
   ax = get("current_axes");
   ax.data_bounds = [0,0;8000,12]; // [0,0;max(SimRes.mt),18];
   ax.tight_limits = "on";
   xlabel("time units");
drawnow();

