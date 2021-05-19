package hyperv.report;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterKeyValue;
import hycallparser.visitors_and_listeners.campaignListeners.CampaignListener;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class HyperVConsoleReporter implements CampaignListener {

    private ResultFileWalker rfw;
    private LogFlagDecoder lfd;

    private long lastEnd;
    private boolean lastEndValid;

    @Override
    public void setParameters(String[] args) {
        if (args.length != 1)
            throw new IllegalArgumentException("Expected only one argument (filepath to binary log file.");

        rfw = new ResultFileWalker(args[0]);
        lfd = new LogFlagDecoder(rfw.read(4));

        if (!lfd.anyLogAvailable())
            System.out.println("No values were logged for this campaign execution!");
    }

    @Override
    public void encodeHypercall(List<ParameterKeyValue> parameters) {
        Map<String, Parameter> args = parameters.stream().collect(Collectors.toMap(ParameterKeyValue::getKey, ParameterKeyValue::getValue));

        System.out.println("Hypercall: ");
        System.out.println("\tName: " + args.get("name").getString());
        if (lfd.exectime)
            System.out.println("\tExec time: " + rfw.read(8)/10.0 + "us");
        if (lfd.timesteps)
            System.out.println("\tExec time: " + (-rfw.read(8) + rfw.read(8)) / 10.0 + "us");
        if (lfd.result)
            System.out.println("\tResult value: " + rfw.read(8));
        if (lfd.output)
            rfw.skipOutputpage();

        System.out.println();
    }

    @Override
    public void encodeDelay(long delayMicroseconds) {
        if (lfd.timesteps) {
            System.out.println("Delay:");
            System.out.println("\tExpected: " + delayMicroseconds + "us");
            System.out.println("\tActual: " + (-rfw.read(8) + rfw.read(8)) / 10.0 + "us");
            System.out.println();
        }
        if (lfd.exectime) {
            System.out.println("Delay:");
            System.out.println("\tExpected: " + delayMicroseconds + "us");
            System.out.println("\tActual: " + rfw.read(8) / 10.0 + "us");
            System.out.println();
        }
    }

    @Override
    public void finish() {
        // do nothing
    }
}
