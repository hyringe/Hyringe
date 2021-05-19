package hycallparser.visitors_and_listeners.campaignListeners;

import hycallparser.parameter.ParameterKeyValue;

import java.util.List;

public class DefaultCampaignListener implements CampaignListener {

    @Override
    public void setParameters(String[] args) {
        System.out.println("========= CAMPAIGN START =========");
    }

    @Override
    public void encodeHypercall(List<ParameterKeyValue> parameters) {
        System.out.println("Hypercall with parameters: " + parameters);
    }

    @Override
    public void encodeDelay(long delayMicroseconds) {
        System.out.println("Delay for " + delayMicroseconds + "us");
    }

    @Override
    public void finish() {
        System.out.println("========== CAMPAIGN END ==========");
    }
}