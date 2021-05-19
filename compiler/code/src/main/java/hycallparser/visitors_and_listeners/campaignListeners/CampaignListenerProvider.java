package hycallparser.visitors_and_listeners.campaignListeners;

import java.util.Collection;
import java.util.Optional;

public interface CampaignListenerProvider {

    Collection<String> getAllNames();
    Optional<CampaignListener> provide(String name);
}