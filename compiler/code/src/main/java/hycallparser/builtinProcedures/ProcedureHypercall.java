package hycallparser.builtinProcedures;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterKeyValue;
import hycallparser.parameter.ParameterNone;
import hycallparser.visitors_and_listeners.campaignListeners.CampaignListenerManager;

import java.util.List;
import java.util.stream.Collectors;

public class ProcedureHypercall implements Procedure {

    @Override
    public String getName() {
        return "hcall";
    }

    @Override
    public int getParameterCount() {
        return 1;
    }

    @Override
    public Parameter perform(List<Parameter> args) {
        Parameter p = args.get(0);
        if (!p.isList())
            throw new IllegalArgumentException("Procedure \"hcall\" expects a list of key-value pairs");

        List<ParameterKeyValue> l = p.getValues().stream().filter(Parameter::isKeyValue).map(pa -> (ParameterKeyValue)pa).collect(Collectors.toList());
        if (p.getValues().size() != l.size())
            throw new IllegalArgumentException("Procedure \"hcall\" expects a list of key-value pairs");

        CampaignListenerManager.getActiveCampaignListener().encodeHypercall(l);

        return ParameterNone.instance();
    }
}
