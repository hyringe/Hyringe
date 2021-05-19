package hycallparser.visitors_and_listeners.campaignListeners;

import hycallparser.parameter.ParameterKeyValue;

import java.util.List;

public interface CampaignListener {

    void setParameters(String[] args);

    void encodeHypercall(List<ParameterKeyValue> parameters);
    void encodeDelay(long delayMicroseconds);

    void finish();
}