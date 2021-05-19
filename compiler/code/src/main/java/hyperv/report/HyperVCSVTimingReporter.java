package hyperv.report;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterKeyValue;
import hycallparser.visitors_and_listeners.campaignListeners.CampaignListener;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class HyperVCSVTimingReporter implements CampaignListener {

    private ResultFileWalker rfw;
    private LogFlagDecoder lfd;
    private RelativeTimeTracker rtt;
    private PrintWriter pw;

    @Override
    public void setParameters(String[] args) {
        rfw = new ResultFileWalker(args[0]);
        if (args.length == 1) {

        } else if (args.length == 2) {
            File file = new File(args[1]);
            file.getParentFile().mkdirs();

            try {
                pw = new PrintWriter(file);
            } catch (FileNotFoundException e) {
                throw new IllegalArgumentException("File \"" + args[1] + "\" not found!");
            }
        } else
            throw new IllegalArgumentException("Expected one or two arguments: the filepath of the binary result file and optionally a filepath to the output CSV  file");

        lfd = new LogFlagDecoder(rfw.read(4));
        if (lfd.exectime)
            outputLine("Event,exectime");
        else if (lfd.timesteps)
            outputLine("Event,start,end");
        else
            throw new IllegalArgumentException("Either exectimes or timestamps have to be enabled!");

        rtt = new RelativeTimeTracker();
    }

    private void outputLine(String output) {
        if (pw == null)
            System.out.println(output);
        else
            pw.println(output);
    }

    @Override
    public void encodeHypercall(List<ParameterKeyValue> parameters) {
        Map<String, Parameter> args = parameters.stream().collect(Collectors.toMap(ParameterKeyValue::getKey, ParameterKeyValue::getValue));

        if (lfd.exectime)
            outputLine("hcall_" + args.get("name").getString() + "," + rfw.read(8)/10.0);
        else if (lfd.timesteps)
            outputLine("hcall_" + args.get("name").getString() + "," + rtt.getRelativeTime(rfw.read(8))/10.0 + "," + rtt.getRelativeTime(rfw.read(8))/10.0);
        if (lfd.result)
            rfw.skip(8);
        if (lfd.output)
            rfw.skipOutputpage();
    }

    @Override
    public void encodeDelay(long delayMicroseconds) {
        if (lfd.exectime)
            outputLine("delay," + rfw.read(8)/10.0);
        else if (lfd.timesteps)
            outputLine("delay," + rtt.getRelativeTime(rfw.read(8))/10.0 + "," + rtt.getRelativeTime(rfw.read(8))/10.0);
    }

    @Override
    public void finish() {
        if (pw != null)
            pw.close();
    }
}
