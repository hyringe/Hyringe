package hycallparser.visitors_and_listeners.campaignListeners;

import hyperv.campaign.HyperVCampaignGenerator;
import hyperv.report.HyperVCSVTimingReporter;
import hyperv.report.HyperVConsoleReporter;

import java.util.Collection;
import java.util.HashMap;
import java.util.Optional;

public class CampaignListenerProviderHyperV implements CampaignListenerProvider {

    private HashMap<String, CampaignListener> listeners;

    public CampaignListenerProviderHyperV() {
        listeners = new HashMap<>();
        listeners.put("hyperv-compile", new HyperVCampaignGenerator());
        listeners.put("hyperv-report-console", new HyperVConsoleReporter());
        listeners.put("hyperv-report-csv", new HyperVCSVTimingReporter());
    }

    @Override
    public Collection<String> getAllNames() {
        return listeners.keySet();
    }

    @Override
    public Optional<CampaignListener> provide(String name) {
        return Optional.ofNullable(listeners.get(name));
    }
}