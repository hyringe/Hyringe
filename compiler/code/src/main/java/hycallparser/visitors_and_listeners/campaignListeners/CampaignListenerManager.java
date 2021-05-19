package hycallparser.visitors_and_listeners.campaignListeners;

import org.apache.commons.lang3.ArrayUtils;
import org.reflections.Reflections;

import java.util.*;
import java.util.stream.Collectors;

public class CampaignListenerManager {

    private static List<CampaignListenerProvider> clps;

    static {
        clps = new ArrayList<>();
        for (Class c : new Reflections("hycallparser.visitors_and_listeners.campaignListeners").getSubTypesOf(CampaignListenerProvider.class)) {
            try {
                CampaignListenerProvider clp = (CampaignListenerProvider)c.getConstructor().newInstance();
                clps.add(clp);
            } catch (Exception e) {
                e.printStackTrace();
                throw new IllegalArgumentException("Exception occured during instantiation of Procedure " + c.getName() + "!");
            }
        }

        List<String> allListeners = clps.stream().map(CampaignListenerProvider::getAllNames).flatMap(Collection::stream).collect(Collectors.toList());
        List<String> duplicates = allListeners.stream().filter(e -> Collections.frequency(allListeners, e) > 1).distinct().collect(Collectors.toList());
        if (!duplicates.isEmpty())
            throw new IllegalArgumentException("There are multipe CampaignListeners with the same name: " + duplicates);
    }

    private CampaignListenerManager() {}

    private static CampaignListener cl = new DefaultCampaignListener();

    public static CampaignListener getActiveCampaignListener() {
        return cl;
    }

    public static void loadCampaignListener(String[] args) {
        if (args.length == 0) {
            System.out.println("No campaign listener specified, will be using default (print to console).");
            printAvailableListeners();
            cl.setParameters(null);
            return;
        }

        clps.stream().map(clp -> clp.provide(args[0])).filter(Optional::isPresent).map(Optional::get).findFirst().ifPresentOrElse(
                c -> {
                    cl = c;
                    cl.setParameters(ArrayUtils.subarray(args, 1, args.length));
                },
                () -> {
                    System.out.println("Listener \"" + args[0] + "\" does not exist.");
                    printAvailableListeners();
                    throw new IllegalArgumentException();
                });
    }

    private static void printAvailableListeners() {
        System.out.println("Available listeners:");
        clps.stream().map(CampaignListenerProvider::getAllNames).flatMap(Collection::stream).forEach(System.out::println);
        System.out.println();
    }
}